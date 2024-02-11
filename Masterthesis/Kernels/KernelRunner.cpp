#include <cstdlib>
#include <stdexcept>

#include <ocl_buffer.h>
#include <ocl_context.h>
#include <ocl_device.h>
#include <ocl_device_type.h>
#include <ocl_image.h>
#include <ocl_kernel.h>
#include <ocl_platform.h>
#include <ocl_program.h>
#include <ocl_queue.h>
#include <utl_args.h>
#include <utl_matrix.h>
#include <utl_profile_pass.h>
#include <utl_profile_pass_manager.h>


typedef float Type;



class BufferPass : public utl::ProfilePass< Type >
{
public :
  BufferPass( std::istream& source, utl::Dim const& start, utl::Dim const& step, utl::Dim const& end, std::size_t iter = 10 );
    
  double prof( utl::Dim const& ) override;
  
  double ops( utl::Dim const& dim ) override;
  
private :
  typedef utl::Matrix< ValueType, utl::column_major_tag > Matrix; // Volkov uses column-major layout only.
  typedef utl::Zeros< ValueType, utl::column_major_tag > Zeros;
  
  bool          testing_;
  ocl::Platform platform_;
  ocl::Device   device_;
  ocl::Context  context_;
  ocl::Queue    queue_;
  ocl::Program  program_;
};



BufferPass::BufferPass( std::istream& source, utl::Dim const& start, utl::Dim const& step, utl::Dim const& end, std::size_t iter ):
  ProfilePass< ValueType >( "BufferPass", start, step, end, iter ),
  testing_( true ),
  platform_( ocl::device_type::CPU ),
  device_( platform_.device( ocl::device_type::CPU ) ),
  context_( device_ ),
  queue_( (platform_.insert( context_ ), platform_.setActiveContext( context_ ), context_), device_, CL_QUEUE_PROFILING_ENABLE ),
  program_( (context_.setActiveQueue( queue_ ), context_), utl::type::Single )
{
  program_ << source;
  
  std::ostringstream oss;
  oss << "-cl-std=CL1.2 -w -Werror";
  
  ocl::CompileOption const opts( oss.str() );
  
  program_.setCompileOption( ocl::compile_option::FAST_MATH | ocl::compile_option::NO_SIGNED_ZERO | opts );

//   ocl::Kernel& kernel( program_.kernel( "gemm_simple" ) );
//   kernel.specialize( "gemm", utl::getType< ValueType >().name() );
  
  program_.build();
  
  if ( program_.isBuilt() )
  {
    context_.setActiveProgram( program_ );
    
    ocl::Kernel& kernel( program_.kernel( "gemm_simple", utl::type::Single ) );
    
    if ( !kernel.created() )
    {
      throw std::runtime_error( "kernel not created" );
    }
  }
  else
  {
    throw std::runtime_error( "program not built" );
  }
}



double BufferPass::prof( utl::Dim const& dim )
{
  std::size_t const N = dim[0];
  std::size_t const M = dim[1];
  std::size_t const L = dim[2];
  
  Zeros result( N, M );
  
  Matrix lhs( N, L );
  Matrix rhs( L, M );
  
  // for ( size_t i = 0; i < N * L; ++i ) lhs[i] = i + 1; for ( size_t i = 0; i < L * M; ++i ) rhs[i] = i + 1;
  for ( size_t i = 0; i < N * L; ++i ) lhs[i] = i % L;
  for ( size_t i = 0; i < L * M; ++i ) rhs[i] = i / L;
  
  std::chrono::nanoseconds totalRuntime{ 0 };
  
  ocl::Kernel& kernel( program_.kernel( "gemm_simple", utl::type::Single ) );
      
  kernel.setWorkSize( 1, 1, M, N );
  
  size_t constexpr typeSize = sizeof (Type);
  size_t const numResultBytes = typeSize * result.size();
  size_t const numLhsBytes = typeSize * lhs.size();
  size_t const numRhsBytes = typeSize * rhs.size();
  
  ocl::Buffer bufResult( context_, numResultBytes, ocl::Buffer::WriteOnly ),
              bufLhs( context_, numLhsBytes, ocl::Buffer::ReadOnly ),
              bufRhs( context_, numRhsBytes, ocl::Buffer::ReadOnly );

  for ( std::size_t i = 0; i < this->_iter; ++i )
  {
    // Copy data from host to device.
    ocl::Event const lhsWritten = bufLhs.writeAsync( queue_, 0u, lhs.data(), numLhsBytes );
    ocl::Event const rhsWritten = bufRhs.writeAsync( queue_, 0u, rhs.data(), numRhsBytes );
    
    ocl::EventList operandsWritten;
    operandsWritten << lhsWritten << rhsWritten;
    
    unsigned int lhsOffset = 0, rhsOffset = 0, resOffset = 0;
    
    // Matrix is column major
    unsigned int lhsStrideX = L, rhsStrideX = M, resStrideX = M;
    unsigned int lhsStrideY = 1, rhsStrideY = 1, resStrideY = 1;
    
    // Execute kernel when both operands have been loaded.
    ocl::Event const multiplyDone = kernel( queue_, operandsWritten,
                                            bufLhs.id(),
                                            bufRhs.id(),
                                            bufResult.id(),
                                            static_cast< unsigned int >( L ),
                                            lhsOffset,
                                            rhsOffset,
                                            resOffset,
                                            lhsStrideX,
                                            rhsStrideX,
                                            resStrideX,
                                            lhsStrideY,
                                            rhsStrideY,
                                            resStrideY
                                          );
    
    // Copy result from device to host.
    ocl::Event const resultRead = bufResult.readAsync( queue_, 0u, result.data(), numResultBytes, ocl::EventList( multiplyDone ) );
    
    // Wait for all commands being executed.
    queue_.finish();
    
    size_t const kernelRuntime_ns = multiplyDone.finishTime() - multiplyDone.startTime();

    totalRuntime += std::chrono::nanoseconds( kernelRuntime_ns );
  }
  
   if( testing_ )
  {
    auto const ref = lhs * rhs;
    auto const diff = result - ref;
    auto const iMax = std::max_element( diff.begin(), diff.end(), []( Type a, Type b ){
      return std::fabs( a ) < std::fabs( b );
    } );
    
    std::cout /*<< lhs << '*' << rhs << " = " */ << "ref = " << ref << std::endl;
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



double BufferPass::ops( utl::Dim const& dim )
{
  // N * M * (L + (L - 1))
  return dim[0] * dim[1] * (2.0 * dim[2] - 1.0);
}



class ImagePass : public utl::ProfilePass< Type >
{
public :
  ImagePass( std::istream& source, utl::Dim const& start, utl::Dim const& step, utl::Dim const& end, std::size_t iter = 10 );
    
  double prof( utl::Dim const& ) override;
  
  double ops( utl::Dim const& dim ) override;
  
private :
  typedef utl::Matrix< ValueType, utl::column_major_tag > Matrix; // Volkov uses column-major layout only.
  typedef utl::Zeros< ValueType, utl::column_major_tag > Zeros;
  
  bool          testing_;
  ocl::Platform platform_;
  ocl::Device   device_;
  ocl::Context  context_;
  ocl::Queue    queue_;
  ocl::Program  program_;
};



ImagePass::ImagePass( std::istream& source, utl::Dim const& start, utl::Dim const& step, utl::Dim const& end, std::size_t iter ):
  ProfilePass< ValueType >( "ImagePass", start, step, end, iter ),
  testing_( true ),
  platform_( ocl::device_type::CPU ),
  device_( platform_.device( ocl::device_type::CPU ) ),
  context_( device_ ),
  queue_( (platform_.insert( context_ ), platform_.setActiveContext( context_ ), context_), device_, CL_QUEUE_PROFILING_ENABLE ),
  program_( (context_.setActiveQueue( queue_ ), context_), utl::type::Single )
{
  program_ << source;
  
  std::ostringstream oss;
  oss << "-cl-std=CL1.2 -w -Werror";
  
  ocl::CompileOption const opts( oss.str() );
  
  program_.setCompileOption( ocl::compile_option::FAST_MATH | ocl::compile_option::NO_SIGNED_ZERO | opts );

//   ocl::Kernel& kernel( program_.kernel( "gemm" ) );
//   kernel.specialize( "gemm", utl::getType< ValueType >().name() );
  
  program_.build();
  
  if ( program_.isBuilt() )
  {
    context_.setActiveProgram( program_ );
    
    ocl::Kernel& kernel( program_.kernel( "gemm_img" ) );
    
    if ( !kernel.created() )
    {
      throw std::runtime_error( "kernel not created" );
    }
  }
  else
  {
    throw std::runtime_error( "program not built" );
  }
}



double ImagePass::prof( utl::Dim const& dim )
{
  if ( !device_.imageSupport() )
    return 0.0;
  
  std::size_t const N = dim[0];
  std::size_t const M = dim[1];
  std::size_t const L = dim[2];
  
  Zeros result( N, M );
  
  Matrix lhs( N, L );
  Matrix rhs( L, M );
  
  // for ( size_t i = 0; i < N * L; ++i ) lhs[i] = i + 1; for ( size_t i = 0; i < L * M; ++i ) rhs[i] = i + 1;
  for ( size_t i = 0; i < N * L; ++i ) lhs[i] = i % L;
  for ( size_t i = 0; i < L * M; ++i ) rhs[i] = i / L;
  
  std::chrono::nanoseconds totalRuntime{ 0 };
  
  ocl::Kernel& kernel( program_.kernel( "gemm_img" ) );
      
  kernel.setWorkSize( 1, 1, M, N );
  
//   size_t constexpr typeSize = sizeof (Type);
//   size_t const numResultBytes = typeSize * result.size();
//   size_t const numLhsBytes = typeSize * lhs.size();
//   size_t const numRhsBytes = typeSize * rhs.size();
  
  ocl::Image imgResult( context_, M, N, ocl::Image::Float, ocl::Image::A, ocl::Image::WriteOnly ),
             imgLhs( context_, L, N, ocl::Image::Float, ocl::Image::A, ocl::Image::ReadOnly ),
             imgRhs( context_, M, L, ocl::Image::Float, ocl::Image::A, ocl::Image::ReadOnly );
             
  /*std::unique_ptr< float[] > lhsData( new float[L * N * 4] ), rhsData( new float[M * L * 4] );
  
  // OpenCL images have row major data layout.
  for ( size_t i = 0; i < N * L; ++i )
  {
    lhsData[i * 4 + 0] = 0.0f;
    lhsData[i * 4 + 1] = 0.0f;
    lhsData[i * 4 + 2] = 0.0f;
    lhsData[i * 4 + 3] = lhs[i];
  }
  
  // 0 3    0 1
  // 1 4    2 3
  // 2 5    4 5
  
  for ( size_t i = 0; i < L * M; ++i )
  {
    rhsData[i * 4 + 0] = 0.0f;
    rhsData[i * 4 + 1] = 0.0f;
    rhsData[i * 4 + 2] = 0.0f;
    rhsData[i * 4 + 3] = rhs[i];
  }*/

  for ( std::size_t i = 0; i < this->_iter; ++i )
  {
    // Copy data from host to device.
    size_t           origin[] = { 0u, 0u, 0u };
    size_t const     lhsRegion[] = { L, N, 1u };
    size_t const     rhsRegion[] = { M, L, 1u };
    
    ocl::Event const lhsWritten = imgLhs.writeAsync( queue_, origin, lhs.data(), lhsRegion );
    ocl::Event const rhsWritten = imgRhs.writeAsync( queue_, origin, rhs.data(), rhsRegion );
    
    ocl::EventList operandsWritten;
    operandsWritten << lhsWritten << rhsWritten;
    
    int lhsOffsetX = 0, rhsOffsetX = 0, resOffsetX = 0;
    int lhsOffsetY = 0, rhsOffsetY = 0, resOffsetY = 0;
    
    // Matrix is column major
//     unsigned int lhsStrideX = L, rhsStrideX = M, resStrideX = M;
//     unsigned int lhsStrideY = 1, rhsStrideY = 1, resStrideY = 1;
    
    int lhsTranspose = (int) utl::isRowMajor< decltype( lhs ) >::value,
        rhsTranspose = (int) utl::isRowMajor< decltype( rhs ) >::value,
        resTranspose = (int) utl::isRowMajor< decltype( result ) >::value;
        
    int innerDim = L;
    
    // Execute kernel when both operands have been loaded.
    ocl::Event const multiplyDone = kernel( queue_, operandsWritten,
                                            imgLhs.id(),
                                            imgRhs.id(),
                                            imgResult.id(),
                                            innerDim,
                                            lhsOffsetX,
                                            rhsOffsetX,
                                            resOffsetX,
                                            lhsOffsetY,
                                            rhsOffsetY,
                                            resOffsetY,
                                            lhsTranspose,
                                            rhsTranspose,
                                            resTranspose
                                          );
    
    // Copy result from device to host.
//     std::unique_ptr< float[] > resultData( new float[N * M * 4] );
    size_t const resRegion[] = { M, N, 1u };
    ocl::Event const resultRead = imgResult.readAsync( queue_, origin, result.data(), resRegion, ocl::EventList( multiplyDone ) );
    
    // Wait for all commands being executed.
    queue_.finish();
    
/*    for ( size_t i = 0; i < N * M; ++i )
      result[i] = resultData[i * 4 + 3];*/
    
    size_t const kernelRuntime_ns = multiplyDone.finishTime() - multiplyDone.startTime();

    totalRuntime += std::chrono::nanoseconds( kernelRuntime_ns );
  }
  
   if( testing_ )
  {
    auto const ref = lhs * rhs;
    auto const diff = result - ref;
    auto const iMax = std::max_element( diff.begin(), diff.end(), []( Type a, Type b ){
      return std::fabs( a ) < std::fabs( b );
    } );
    
    std::cout /*<< lhs << '*' << rhs << " = " */ << "ref = " << ref << std::endl;
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



double ImagePass::ops( utl::Dim const& dim )
{
  // N * M * (L + (L - 1))
  return dim[0] * dim[1] * (2.0 * dim[2] - 1.0);
}



int main( int argc, char** argv )
{
  utl::Args args( argc, argv );
  
  if ( args.size() == 2 )
  {
    std::string const filename( args.toString( 1 ) );
    std::ifstream file( filename );
    
    if ( file.is_open() )
    {
      utl::ProfilePassManager< Type > mgr;
  
//       mgr << std::make_shared<BufferPass>( file, utl::Dim( 256, 256, 256 ), utl::Dim( 16, 16, 16 ), utl::Dim( 256, 256, 256 ) );
      mgr << std::make_shared<ImagePass>( file, utl::Dim( 255, 255, 255 ), utl::Dim( 16, 16, 16 ), utl::Dim( 256, 256, 256 ) );
  
      mgr.run();
      mgr.write( std::cout );
    }
    else
    {
      std::cerr << "Failed opening file " << filename << std::endl;
    }
  }
  else
  {
    std::cout << "Usage: " << args.at( 0 ) << " <kernel.cl>" << std::endl;
  }
  
  return EXIT_SUCCESS;
}
