/**
 * Simple matrix-matrix multiplication kernel using 
 * buffers rather than image objects.
 * 
 * Invoke the kernel with a two-dimensional index space
 * with a work item per element of the result matrix.
 *
 * @param lhs Left-hand side (sub-) matrix.
 * @param rhs Right-hand side (sub-) matrix.
 * @param res Result (sub-) matrix.
 * @param innerDim Inner dimension of @c lhs and @c rhs.
 * @param lhsOffset Offset into @c lhs where the (sub-) matrix begins.
 * @param rhsOffset The same for @c rhs.
 * @param resOffset The same for @c res.
 * @param lhsStrideX Distance between consecutive @c lhs values in horizontal direction.
 * @param rhsStrideX The same for @c rhs.
 * @param resStrideX The same for @c res.
 * @param lhsStrideY Distance between consecutive @c lhs values in vertical direction.
 * @param rhsStrideY The same for @c rhs.
 * @param resStrideY The same for @c res.
 * 
 * @tparam T Datatype of the matrix operands.
 */
template<class T>
__kernel void gemm(
  __global T const* restrict lhs,
  __global T const* restrict rhs,
  __global T* const restrict res,
  uint const        innerDim,
  uint const        lhsOffset,
  uint const        rhsOffset,
  uint const        resOffset,
  uint const        lhsStrideX,
  uint const        rhsStrideX,
  uint const        resStrideX,
  uint const        lhsStrideY,
  uint const        rhsStrideY,
  uint const        resStrideY
)
{
  size_t const gx  = get_global_id( 0u );
  size_t const gy  = get_global_id( 1u );
  T            tmp = (T) 0;
  
  for ( size_t k = 0u; k < innerDim; ++k )
    tmp += lhs[lhsOffset + gy * lhsStrideY + k * lhsStrideX] *
           rhs[rhsOffset + gx * rhsStrideX + k * rhsStrideY];
  
  res[resOffset + gy * resStrideY + gx * resStrideX] = tmp;
}



/**
 * @see gemm().
 */
template<class T>
__kernel void gemm_basic_algorithm(
  __global T const* restrict lhs,
  __global T const* restrict rhs,
  __global T* const restrict res,
  uint const        K,
  uint const        lhsOffset,
  uint const        rhsOffset,
  uint const        resOffset,
  uint const        lhsStrideX,
  uint const        rhsStrideX,
  uint const        resStrideX,
  uint const        lhsStrideY,
  uint const        rhsStrideY,
  uint const        resStrideY
)
{
}



/**
 * @see gemm().
 */
template<class T>
__kernel void gemm_double_buffering(
  __global T const* restrict lhs,
  __global T const* restrict rhs,
  __global T* const restrict res,
  uint const        K,
  uint const        lhsOffset,
  uint const        rhsOffset,
  uint const        resOffset,
  uint const        lhsStrideX,
  uint const        rhsStrideX,
  uint const        resStrideX,
  uint const        lhsStrideY,
  uint const        rhsStrideY,
  uint const        resStrideY
)
{
}



/**
 * @see gemm().
 */
template<class T>
__kernel void gemm_pipelining(
  __global T const* restrict lhs,
  __global T const* restrict rhs,
  __global T* const restrict res,
  uint const        K,
  uint const        lhsOffset,
  uint const        rhsOffset,
  uint const        resOffset,
  uint const        lhsStrideX,
  uint const        rhsStrideX,
  uint const        resStrideX,
  uint const        lhsStrideY,
  uint const        rhsStrideY,
  uint const        resStrideY
)
{
}



/**
 * Simple kernel for matrix-matrix multiplication using image objects.
 * 
 * Invoke the kernel with a two-dimensional index space
 * with a work-item per element of the result matrix.
 * 
 * This kernel is not templatized as that would require 
 * a semantic preprocessor that is currently not available
 * in the OpenCL-Wrapper library.
 * 
 * @param lhs Left-hand side (sub-) matrix.
 * @param rhs Right-hand side (sub-) matrix.
 * @param res Result (sub-) matrix.
 * @param innerDim Inner dimension of @c lhs and @c rhs.
 * @param lhsOffsetX First texel of the @c lhs (sub-) matrix in x direction.
 * @param rhsOffsetX The same for @c rhs.
 * @param resOffsetX The same for @c res.
 * @param lhsOffsetY First texel of the @c lhs (sub-) matrix in y direction.
 * @param rhsOffsetY The same for @c rhs.
 * @param resOffsetY The same for @c res.
 * @param lhsTranspose Indicates whether the @c lhs matrix is transposed.
 * @param rhsTranspose The same for @c rhs.
 * @param resTranspose The same for @c res.
 */
__kernel void gemm_img(
  read_only image2d_t  lhs,
  read_only image2d_t  rhs,
  write_only image2d_t res,
  int const            innerDim,
  int const            lhsOffsetX,
  int const            rhsOffsetX,
  int const            resOffsetX,
  int const            lhsOffsetY,
  int const            rhsOffsetY,
  int const            resOffsetY,
  int const            lhsTranspose,
  int const            rhsTranspose,
  int const            resTranspose
)
{
  sampler_t const sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;
  
  size_t const gx  = get_global_id( 0u );
  size_t const gy  = get_global_id( 1u );
  float4       tmp = (float4)(0.0f,0.0f,0.0f,0.0f);
  
  int2 const lhsOffset = (int2)( lhsOffsetX, lhsOffsetY );
  int2 const rhsOffset = (int2)( rhsOffsetX, rhsOffsetY );
  
  for ( int k = 0; k < innerDim; ++k )
  {
    int2 const lIdx = (int2)( k, gy );
    int2 const rIdx = (int2)( gx, k );
    
    float4 l = read_imagef( lhs, sampler, lhsOffset + select( lIdx.yx, lIdx, (int2)( lhsTranspose, lhsTranspose ) ) );
    float4 r = read_imagef( rhs, sampler, rhsOffset + select( rIdx.yx, rIdx, (int2)( rhsTranspose, rhsTranspose ) ) );
    
    tmp.w += l.w * r.w;
  }
  
  int2 const resOffset = (int2)( resOffsetX, resOffsetY );
  int2 const resIdx    = (int2)( gx, gy );
  
  write_imagef( res, resOffset + select( resIdx, resIdx.yx, (int2)( resTranspose, resTranspose ) ), tmp );
}



/**
 * @see gemm_img().
 */
__kernel void gemm_img_basic_algorithm(
  read_only image2d_t  lhs,
  read_only image2d_t  rhs,
  write_only image2d_t res,
  int const            innerDim,
  int const            lhsOffsetX,
  int const            rhsOffsetX,
  int const            resOffsetX,
  int const            lhsOffsetY,
  int const            rhsOffsetY,
  int const            resOffsetY,
  int const            lhsTranspose,
  int const            rhsTranspose,
  int const            resTranspose
)
{
}



/**
 * @see gemm_img().
 */
__kernel void gemm_img_double_buffering(
  read_only image2d_t  lhs,
  read_only image2d_t  rhs,
  write_only image2d_t res,
  int const            innerDim,
  int const            lhsOffsetX,
  int const            rhsOffsetX,
  int const            resOffsetX,
  int const            lhsOffsetY,
  int const            rhsOffsetY,
  int const            resOffsetY,
  int const            lhsTranspose,
  int const            rhsTranspose,
  int const            resTranspose
)
{
}



/**
 * @see gemm_img().
 */
__kernel void gemm_img_pipelining(
  read_only image2d_t  lhs,
  read_only image2d_t  rhs,
  write_only image2d_t res,
  int const            innerDim,
  int const            lhsOffsetX,
  int const            rhsOffsetX,
  int const            resOffsetX,
  int const            lhsOffsetY,
  int const            rhsOffsetY,
  int const            resOffsetY,
  int const            lhsTranspose,
  int const            rhsTranspose,
  int const            resTranspose
)
{
}
