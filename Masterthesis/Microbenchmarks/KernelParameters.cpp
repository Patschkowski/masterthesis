/**
 * This microbenchmark measures the impact of the number of kernel parameters
 * on the kernel execution time.
 */

#include <cassert>
#include <cstdlib>
#include <memory>
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



class KernelLaunchOverheadProfiler : public utl::ProfilePass< float >
{
public :  
  KernelLaunchOverheadProfiler( utl::Dim const& start, utl::Dim const& step, utl::Dim const& end, size_t numIterations = NumIterations ):
    utl::ProfilePass< float >( "KernelLaunchOverhead", start, step, end, numIterations ),
    platform_( ocl::device_type::CPU ),
    device_( platform_.device( ocl::device_type::CPU ) ),
    context_( device_ ),
    queue_( (platform_.insert( context_ ), platform_.setActiveContext( context_ ), context_), device_, CL_QUEUE_PROFILING_ENABLE )
  {
    context_.setActiveQueue( queue_ );
  }
  
  double prof( utl::Dim const& dim ) override
  {
    assert( dim.size() >= 1u );
    
    ocl::Program program( context_, utl::getType< ValueType >() );
    
    int const numArgs = dim[0] - 1;
    
    program << buildSource( numArgs );
//     program.print();
    
    ocl::CompileOption const opts( "-cl-std=CL1.1 -w -Werror" );
    
    program.setCompileOption( ocl::compile_option::FAST_MATH | ocl::compile_option::NO_SIGNED_ZERO | opts );
    program.build();
    
    if ( program.isBuilt() )
    {
      context_.setActiveProgram( program );
      
      ocl::Kernel& kernel( program.kernel( "launch_overhead" ) );
      
      if ( kernel.created() )
      {
        // Don't care about wavefront/warp sizes here.
        kernel.setWorkSize( 1, 1 );
        
        std::vector< ocl::Event > evts( this->_iter );
        ocl::EventList allKernelsExecuted;
        
        std::vector< int > parameters( numArgs, 42 );
        
        for ( auto i = 0u; i < this->_iter; ++i )
        {          
          for ( auto j = 0; j < numArgs; ++j )
          {
            kernel.setArg( j, parameters[j] );
          }
          
          evts[i] = kernel( queue_ );
          
          allKernelsExecuted << evts[i];
        }
        
        // Wait for all kernels to finish.
        queue_.barrier( allKernelsExecuted );
        
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
  
  double ops( utl::Dim const& /* dim */ ) override
  {
    return 0.0;
  }
  
private :
  static std::string buildSource( int numArgs )
  {
    assert( numArgs >= 0 );
    
    std::ostringstream oss;
    
    oss << "__kernel void launch_overhead(";
        
    for ( int i = 0; i < numArgs; ++i )
    {
      if ( i > 0 )
        oss << ',';
      
      oss << " int arg" << i;
    }
    
    oss << ")\n{}";
    
    return oss.str();
  }
  
  ocl::Platform                               platform_;
  ocl::Device                                 device_;
  ocl::Context                                context_;
  ocl::Queue                                  queue_;
};



int main()
{
  try
  {
    utl::ProfilePassManager< float > mgr;
    
    // Number of kernel parameters.
    utl::Dim start( 1 ), step( 1 ), end( 32 );
    
    mgr << std::make_shared< KernelLaunchOverheadProfiler >( start, step, end );
    
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
