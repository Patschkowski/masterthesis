/**
 * This microbenchmark measures the impact of the number of kernel parameters
 * on the kernel execution time.
 */

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <numeric>
#include <stdexcept>

#include <ocl_buffer.h>
#include <ocl_context.h>
#include <ocl_device.h>
#include <ocl_device_type.h>
#include <ocl_event_list.h>
#include <ocl_kernel.h>
#include <ocl_platform.h>
#include <ocl_program.h>
#include <ocl_query.h>
#include <ocl_queue.h>

#include <utl_profile_pass.h>
#include <utl_profile_pass_manager.h>
#include <utl_type.h>

constexpr size_t NumIterations = 10000u;

constexpr char const kernels[] = R"(
  
template<class T>
__kernel void dot_simple( __global const T* restrict u, __global const T* restrict v, uint N, __global T* const w )
{
  T tmp = 0;
  
  for ( uint i = 0; i < N; ++i )
    tmp += u[i] * v[i];
  
  w[get_global_id(0)] = tmp;
}

template<class T>
__kernel void dot_ptr_arith( __global const T* restrict u, __global const T* restrict v, uint N, __global T* const w )
{
  T tmp = 0;
  
  for ( uint i = 0; i < N; ++i )
    tmp += *u++ * *v++;
  
  w[get_global_id(0)] = tmp;
}


__kernel void dot_fma( __global const float* restrict u, __global const float* restrict v, uint N, __global float* const w )
{
  float tmp = 0.0f;
  
  for ( uint i = 0; i < N; ++i )
    tmp = fma( u[i], v[i], tmp );
  
  w[get_global_id(0)] = tmp;
}

template<class T>
__kernel void dot_vec2( __global const T* restrict u, __global const T* restrict v, uint N, __global T* const w )
{
  T tmp = 0.0f;
  
  for ( uint i = 0; i < N/2; ++i )
    tmp += dot( vload2( i, u ), vload2( i, v ) );
  
  if ( N % 2 == 1 )
    tmp += u[N-1] * v[N-1];
  
  w[get_global_id(0)] = tmp;
}

template<class T>
__kernel void dot_vec4( __global const T* restrict u, __global const T* restrict v, uint N, __global T* const w )
{
  T tmp = 0.0f;
  
  for ( uint i = 0; i < N/4; ++i )
    tmp += dot( vload4( i, u ), vload4( i, v ) );
  
  for ( uint i = N%4; i > 0; --i )
    tmp += u[N-i] * v[N-i];
  
  w[get_global_id(0)] = tmp;
}

__kernel void dot_fma_vec2( __global const float* restrict u, __global const float* restrict v, uint N, __global float* const w )
{
  float2 tmp = 0.0f;
  
  for ( uint i = 0; i < N/2; ++i )
    tmp = fma( vload2( i, u ), vload2( i, v ), tmp );
  
  if ( N % 2 == 1 )
    tmp.x += u[N-1] * v[N-1];
  
  w[get_global_id(0)] = tmp.x + tmp.y;
}

__kernel void dot_fma_vec4( __global const float* restrict u, __global const float* restrict v, uint N, __global float* const w )
{
  float4 tmp = 0.0f;
  
  for ( uint i = 0; i < N/4; ++i )
    tmp = fma( vload4( i, u ), vload4( i, v ), tmp );
  
  for ( uint i = N%4; i > 0; --i )
    tmp.x += u[N-i] * v[N-i];
  
  w[get_global_id(0)] = tmp.x + tmp.y + tmp.z + tmp.w;
}
  )";


template< typename T >
class DotProductProfiler : public utl::ProfilePass< T >
{
public :
  using typename utl::ProfilePass< T >::ValueType;
  
  DotProductProfiler( std::string const& kernelName, bool isTemplatized, utl::Dim const& start, utl::Dim const& step, utl::Dim const& end, size_t numIterations = NumIterations ):
    utl::ProfilePass< T >( "DotProduct_" + kernelName, start, step, end, numIterations ),
    platform_( ocl::device_type::CPU ),
    device_( platform_.device( ocl::device_type::CPU ) ),
    context_( device_ ),
    queue_( (platform_.insert( context_ ), platform_.setActiveContext( context_ ), context_), device_, CL_QUEUE_PROFILING_ENABLE ),
    kernelName_( kernelName ),
    isTemplatized_( isTemplatized )
  {
    context_.setActiveQueue( queue_ );
  }
  
  double prof( utl::Dim const& dim ) override
  {
    assert( dim.size() >= 1u );
    
    ocl::Program program( context_, utl::getType< ValueType >() );
    
    int const dimension = dim[0] - 1;
    
    program << kernels;
//     program.print();
    
    ocl::CompileOption const opts( "-cl-std=CL1.1 -w -Werror" );
    
    program.setCompileOption( ocl::compile_option::FAST_MATH | ocl::compile_option::NO_SIGNED_ZERO | opts );
    program.build();
    
    if ( program.isBuilt() )
    {
      context_.setActiveProgram( program );
      
      ocl::Kernel* dummyPtr = nullptr;
      if ( isTemplatized_ )
        dummyPtr = &program.kernel( kernelName_, utl::type::Single );
      else
        dummyPtr = &program.kernel( kernelName_ );
      
      ocl::Kernel& kernel( *dummyPtr );
      
      if ( kernel.created() )
      {
        // Don't care about wavefront/warp sizes here.
        size_t numWorkers = device_.maxWorkItemSizes()[0];
        kernel.setWorkSize( 1, numWorkers );
        
        std::vector< ocl::Event > evts( this->_iter );
        ocl::EventList allKernelsExecuted;
        
        size_t const numBytes = dimension * sizeof (ValueType);
        auto const   numResBytes = sizeof (ValueType) * numWorkers;
        
        ocl::Buffer u( context_, numBytes, ocl::Buffer::ReadOnly ), 
                    v( context_, numBytes, ocl::Buffer::ReadOnly ),
                    w( context_, numResBytes, ocl::Buffer::WriteOnly );
                    
        std::vector< ValueType > lhs( dimension, 1 ), rhs( dimension, 1 ), res( numWorkers );
        
        auto result = std::inner_product(
          lhs.begin(), lhs.end(), rhs.begin(), static_cast< ValueType >( 0 )
        );
        
        for ( auto i = 0u; i < this->_iter; ++i )
        {                   
          ocl::Event const lhsWritten = u.writeAsync( queue_, 0u, lhs.data(), numBytes );
          ocl::Event const rhsWritten = v.writeAsync( queue_, 0u, rhs.data(), numBytes );
    
          ocl::EventList operandsWritten;
          operandsWritten << lhsWritten << rhsWritten;
    
          evts[i] = kernel( queue_, operandsWritten, u.id(), v.id(), dimension, w.id() );
          
          ocl::Event const resultRead = w.readAsync( queue_, 0u, res.data(), numResBytes, ocl::EventList( evts[i] ) );
          
          allKernelsExecuted << evts[i];
        }
        
        // Wait for all kernels to finish.
        queue_.barrier( allKernelsExecuted );
        
        assert( std::count( res.begin(), res.end(), result ) == res.size() );
        
        // Calculate the total execution time.
        double totalRuntime_ns( 0 );

        //for ( auto const& evt : allKernelsExecuted.events() )
        for ( size_t i = 0; i < allKernelsExecuted.size(); ++i )
        {
          auto const start = allKernelsExecuted.at(i).startTime();
          auto const end   = allKernelsExecuted.at(i).finishTime();
          auto const kernelRuntime_ns = end - start;

          totalRuntime_ns += kernelRuntime_ns;
        }
                
        // Return average time in seconds.
        return (static_cast< double >( totalRuntime_ns ) / this->_iter) / 1000000000.0;
        //return std::chrono::duration_cast< std::chrono::microseconds >( totalRuntime /* this->_iter */).count();
      }
      else
      {
        throw std::runtime_error( "kernel not created" );
      }
    }
    else
    {
      throw std::runtime_error( "program not built" );
    }
  }
  
  double ops( utl::Dim const& dim ) override
  {
    return 2 * dim[0] - 1;
  }
  
private :  
  ocl::Platform                               platform_;
  ocl::Device                                 device_;
  ocl::Context                                context_;
  ocl::Queue                                  queue_;
  std::string                                 kernelName_;
  bool isTemplatized_;
};



int main()
{
  try
  {
    utl::ProfilePassManager< float > mgr;
    
    // Number of kernel parameters.
    utl::Dim start( 1 ), step( 1 ), end( 1 << 20 );
    
    mgr << std::make_shared< DotProductProfiler< float > >( "dot_simple", true, start, step, end );
    mgr << std::make_shared< DotProductProfiler< float > >( "dot_ptr_arith", true, start, step, end );
    mgr << std::make_shared< DotProductProfiler< float > >( "dot_fma", false, start, step, end );
    mgr << std::make_shared< DotProductProfiler< float > >( "dot_vec2", true, start, step, end );
    mgr << std::make_shared< DotProductProfiler< float > >( "dot_vec4", true, start, step, end );
    mgr << std::make_shared< DotProductProfiler< float > >( "dot_fma_vec2", false, start, step, end );
    mgr << std::make_shared< DotProductProfiler< float > >( "dot_fma_vec4", false, start, step, end );
    
    mgr.run();
    mgr.write( std::cout );
  }
  catch ( std::exception& e )
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}
