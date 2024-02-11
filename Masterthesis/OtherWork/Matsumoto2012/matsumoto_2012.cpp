#include <cstdlib>
#include <stdexcept>

#include <ocl_buffer.h>
#include <ocl_context.h>
#include <ocl_device.h>
#include <ocl_device_type.h>
#include <ocl_kernel.h>
#include <ocl_platform.h>
#include <ocl_program.h>
#include <ocl_queue.h>
#include <utl_args.h>
#include <utl_profile_pass.h>
#include <utl_profile_pass_manager.h>

#include "utl_matrix2.hpp"

#include "BasicTemplate.hpp"
#include "DoubleBufferingTemplate.hpp"
#include "PipeliningTemplate.hpp"

typedef float Type;


template< typename Matrix >
class Matsumoto2012Pass : public utl::ProfilePass< typename Matrix::value_type >
{
public :
  Matsumoto2012Pass( std::unique_ptr< KernelTemplate< Matrix > >&& kernelTemplate, bool testing, utl::Dim const& start, utl::Dim const& step, utl::Dim const& end, std::size_t iter = 10 ):
    utl::ProfilePass< typename Matrix::value_type >( "Matsumoto2012", start, step, end, iter ),
    kernelTemplate_( std::move( kernelTemplate ) ),
    testing_( testing ),
    platform_( ocl::device_type::CPU ),
    device_( platform_.device( ocl::device_type::CPU ) ),
    context_( device_ ),
    queue_( (platform_.insert( context_ ), platform_.setActiveContext( context_ ), context_), device_, CL_QUEUE_PROFILING_ENABLE )
  {
    context_.setActiveQueue( queue_ );
  }
  
  // NOTE Compared to our normal nomenklatur N and M are swapped.
  double prof( utl::Dim const& dim ) override
  {
    std::size_t const N = dim[0];
    std::size_t const M = dim[1];
    std::size_t const L = dim[2];
    
    Matrix lhs(    M, L, kernelTemplate_->Mwg(), kernelTemplate_->Kwg() ),
           rhs(    L, N, kernelTemplate_->Kwg(), kernelTemplate_->Nwg() ),
           result( M, N, kernelTemplate_->Mwg(), kernelTemplate_->Nwg() );
    
    std::ostringstream oss;
    kernelTemplate_->generate( oss, lhs, rhs, result );
    
    ocl::Program program( context_, utl::getType< typename Matrix::value_type >() );
        
    std::string const srcFilename = "/home/fpatschkowski/Projekte/kernel.cl";
    
    {
      std::ofstream srcFile( srcFilename );
      if ( srcFile.is_open() )
      {
        srcFile << oss.str();
      }
      else
      {
        throw std::runtime_error( "Failed to write kernel to " + srcFilename );
      }
    }
    
    {
      std::ifstream srcFile( srcFilename );
      if ( srcFile.is_open() )
      {
        program << srcFile;
//         program.print( std::cout );
      }
      else
      {
        throw std::runtime_error( "Failed to open kernel from " + srcFilename );
      }
    }
        
    ocl::CompileOption const opts( "-cl-std=CL1.2 -w -Werror -g -O0"/*"-g -s \"" + srcFilename + '\"'*/ );
    
    program.setCompileOption( ocl::compile_option::FAST_MATH | ocl::compile_option::NO_SIGNED_ZERO | opts );
    program.build();
    
    if ( program.isBuilt() )
    {
      context_.setActiveProgram( program );
      
      ocl::Kernel& kernel( program.kernel( kernelTemplate_->name() ) );
      
      if ( kernel.created() )
      {       
        for ( size_t i = 0; i < M * L; ++i ) lhs[i] = i % L;
//         for ( auto i = 0u; i < M; ++i ) for ( auto j = 0u; j < L; ++j ) lhs.at( j, i ) = i == j ? 1 : 0; 
        for ( size_t i = 0; i < L * N; ++i ) rhs[i] = i % (L+1) == 0 ? 1 : 0;//i / L;
        for ( size_t i = 0; i < M * N; ++i ) result[i] = 0;
        
        auto const resCopy = result;
        
        std::chrono::nanoseconds totalRuntime{ 0 };
        
        auto const localX = kernelTemplate_->NdimC(), localY = kernelTemplate_->MdimC(), globalX = N / kernelTemplate_->Nwg() * kernelTemplate_->NdimC(), globalY = M / kernelTemplate_->Mwg() * kernelTemplate_->MdimC();
      
        kernel.setWorkSize( localX, localY, globalX, globalY );
        
        size_t constexpr typeSize       = sizeof (typename Matrix::value_type);
        size_t const     numResultBytes = typeSize * result.size();
        size_t const     numLhsBytes    = typeSize * lhs.size();
        size_t const     numRhsBytes    = typeSize * rhs.size();
        
        ocl::Buffer bufResult( context_, numResultBytes, ocl::Buffer::WriteOnly ),
                    bufLhs( context_, numLhsBytes, ocl::Buffer::ReadOnly ),
                    bufRhs( context_, numRhsBytes, ocl::Buffer::ReadOnly );
                    
        Type const alpha = 1, beta = 0;

        for ( std::size_t i = 0; i < this->_iter; ++i )
        {
          // Copy data from host to device.
          ocl::Event const lhsWritten = bufLhs.writeAsync( queue_, 0u, lhs.data(), numLhsBytes );
          ocl::Event const rhsWritten = bufRhs.writeAsync( queue_, 0u, rhs.data(), numRhsBytes );
          
          ocl::EventList operandsWritten;
          operandsWritten << lhsWritten << rhsWritten;
          
          // Execute kernel when both operands have been loaded.
          ocl::Event const multiplyDone = kernel( queue_, operandsWritten, alpha, bufLhs.id(), bufRhs.id(), beta, bufResult.id() );
          
          // Copy result from device to host.
          ocl::Event const resultRead = bufResult.readAsync( queue_, 0u, result.data(), numResultBytes, ocl::EventList( multiplyDone ) );
          
          // Wait for all commands being executed.
          queue_.finish();
          
          size_t const kernelRuntime_ns = multiplyDone.finishTime() - multiplyDone.startTime();

          totalRuntime += std::chrono::nanoseconds( kernelRuntime_ns );
        }
        
        if( testing_ )
        {
          /*auto const t1 = alpha * lhs;
          auto const t2 = t1 *rhs;
          auto const t3 = beta * resCopy;
          auto const t4 = t2 + t3;*/
          auto const ref  = alpha * lhs * rhs + beta * resCopy;
          auto const diff = result - ref;
          auto const iMax = std::max_element( diff.begin(), diff.end(), []( Type a, Type b ){
            return std::fabs( a ) < std::fabs( b );
          } );
          
          std::cout << lhs << '*' << rhs << " = " << "ref = " << ref << std::endl;
          std::cout << "result = " << result << std::endl;
          std::cout << "Maximal error: " << *iMax << std::endl;
          
          if ( *iMax != 0 )
          {
            size_t const index = iMax - diff.begin();
            std::cout << "ref[" << index << "] = " << ref[index] << " != result[" << index << "] = " << result[index] << std::endl;
          }
        }
        
        // Return average time.
        return std::chrono::duration_cast< std::chrono::microseconds >( totalRuntime / this->_iter ).count();
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
    // N * M * (L + (L - 1))
    return dim[0] * dim[1] * (2.0 * dim[2] - 1.0);
  }
  
private :
  std::unique_ptr< KernelTemplate< Matrix > > kernelTemplate_;
  bool                                        testing_;
  ocl::Platform                               platform_;
  ocl::Device                                 device_;
  ocl::Context                                context_;
  ocl::Queue                                  queue_;
};


int main( int argc, char** argv )
{  
  utl::Args args( argc, argv );
  
  // For OpenCL printf.
  std::cout.sync_with_stdio();
  std::cerr.sync_with_stdio();
  std::clog.sync_with_stdio();
  
  if ( args.size() == 2 )
  {
    try
    {
      utl::Dim start( 96, 96, 96 ), step( 96, 96, 96 ), end( /*6144, 6144, 6144*/ 96, 96, 96 );
      
      utl::ProfilePassManager< Type > mgr;

#if 0      
      mgr << std::make_shared< Matsumoto2012Pass< utl::Matrix2< Type, utl::column_major_tag, utl::row_major_tag > > >(
        std::unique_ptr< BasicTemplate< utl::Matrix2< Type, utl::column_major_tag, utl::row_major_tag > > >( new BasicTemplate< utl::Matrix2< Type, utl::column_major_tag, utl::row_major_tag > >(
          start[1]
        ) ), args.toBool( 1 ), start, step, end );
#endif
      
#if 0
      mgr << std::make_shared< Matsumoto2012Pass< utl::Matrix2< Type, utl::column_major_tag, utl::row_major_tag > > >(
        std::unique_ptr< PipeliningTemplate< utl::Matrix2< Type, utl::column_major_tag, utl::row_major_tag > > >( new PipeliningTemplate< utl::Matrix2< Type, utl::column_major_tag, utl::row_major_tag > >(
          start[1]
        ) ), args.toBool( 1 ), start, step, end );
#endif

      mgr << std::make_shared< Matsumoto2012Pass< utl::Matrix2< Type, utl::column_major_tag, utl::row_major_tag > > >(
        std::unique_ptr< DoubleBufferingTemplate< utl::Matrix2< Type, utl::column_major_tag, utl::row_major_tag > > >( new DoubleBufferingTemplate< utl::Matrix2< Type, utl::column_major_tag, utl::row_major_tag > >(
          start[1]
        ) ), args.toBool( 1 ), start, step, end );
      
      mgr.run();
      mgr.write( std::cout );
    }
    catch ( std::exception& e )
    {
      std::cerr << e.what() << std::endl;
      return EXIT_FAILURE;
    }
  }
  else
  {
    std::cout << "Usage: " << args.at( 0 ) << " <bool_testing>" << std::endl;
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}
