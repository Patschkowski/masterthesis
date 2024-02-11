#ifndef KERNEL_TEMPLATE_HPP
#define KERNEL_TEMPLATE_HPP

#include <cassert>
#include <cstring>
#include <iosfwd>

#include <utl_type.h>
#include "utl_matrix2.hpp"

/**
 * Enum to model the vector formats supported by OpenCL.
 */
enum class VectorSize 
{ 
  One     = 1, ///< e.g. float
  Two     = 2, ///< e.g. float2
  Four    = 4, ///< e.g. float4
  Eight   = 8, ///< e.g. float8
  Sixteen = 16 ///< e.g. float16
};



/**
 * Kernel template base class.
 * 
 * This class captures all parameters supported by the Matsumoto
 * code generator and gives an interface for generating kernels. 
 */
template< typename Matrix >
class KernelTemplate
{
public :
  /**
   * Generate a kernel for the multiplication of the matrices given.
   * The kernel created computes \f$C = \alpha \cdot A \cdot B + beta \cdot C\f$ .
   * 
   * @param os Stream to write the kernel to.
   * @param lhs Left hand side matrix.
   * @param rhs Right hand side matrix.
   * @param result Result matrix
   * 
   * @par Exception Guarantee
   * strong
   */
  virtual void generate( std::ostream& os, Matrix const& lhs, Matrix const& rhs, Matrix const& result ) const = 0;
  
  /// @defgroup WorkGroupMatrixDimensions Work group matrix dimensions
  /// @{
  
  /**
   * Get the number of rows a work group calculates of the result matrix.
   */
  size_t Mwg() const { return Mwg_; }
  
  /**
   * Set the number of rows a work group calculates of the result matrix.
   */
  void Mwg( size_t mwg ) {  Mwg_ = mwg; }
  
  /**
   * Get the number of columns a work group calculate the of the result matrix.
   */
  size_t Nwg() const { return Nwg_; }
  
  /**
   * Set the number of columns one work group has to calculate.
   */
  void Nwg( size_t nwg ) { Nwg_ = nwg; }
  
  /**
   * Get the size of the inner dimension of the blocks loaded at a time by a work group.
   */
  size_t Kwg() const { return Kwg_; }
  
  /**
   * Set the size of the inner dimension of the blocks work groups load.
   */
  void Kwg( size_t kwg ) { Kwg_ = kwg; }
  
  /// @}
  
  size_t MdimC() const { return MdimC_; }
  
  void MdimC( size_t mdimc ) { assert( Mwg() % mdimc == 0 ); MdimC_ = mdimc; }
  
  size_t NdimC() const { return NdimC_; }
  
  void NdimC( size_t ndimc ) { assert( Nwg() % ndimc == 0 ); NdimC_ = ndimc; }
  
  size_t KdimA() const { return MdimC() * NdimC() / MdimA(); }
  
  size_t KdimB() const { return MdimC() * NdimC() / NdimB(); }
  
  size_t MdimA() const { return MdimA_; }
  
  void MdimA( size_t mdima ) { MdimA_ = mdima; }
  
  size_t NdimB() const { return NdimB_; }
  
  void NdimB( size_t ndimb ) { NdimB_ = ndimb; }
  
  /// @defgroup WorkItemMatrixDimensions Work item matrix dimensions
  /// @{
  
  size_t Nwi() const { return Nwg() / NdimC(); }
  
  size_t Mwi() const { return Mwg() / MdimC(); }
  
  size_t Kwi() const { return Kwi_; }
  
  void Kwi( size_t kwi ) { Kwi_ = kwi; }
  
  /// @}

  /**
   * Get the vector size used in the computation of the inner loop.
   */
  VectorSize vw() const { return vw_; }
  
  /**
   * Set the vector size used in the computation of the inner loop.
   */
  void vw( VectorSize v ) { /* TODO This needs an assert to assure that it fits to Kwi and Mwi and Nwi*/ vw_ = v; }
  
  /**
   * Get name of the kernel. This is the name of the function.
   */
  std::string name() const { return name_; }
  
  /**
   * Set the name of the kernel.
   * 
   * @pre The name must start with a letter or an underscore and must not have others but these characters and digits in it.
   */
  void name( std::string const& nm ) { assert( isValidKernelName( nm ) ); name_ = nm; }

  /// @defgroup WorkItemMatrixDimensionsForLoading Work item matrix dimensions for collective loading.
  /// @{
  
  size_t MwiA() const { return Mwg() / MdimA(); }
  
  size_t KwiA() const { return Kwg() / KdimA(); }
  
  size_t KwiB() const { return Kwg() / KdimB(); }
  
  size_t NwiB() const { return Nwg() / NdimB(); }
  
  /// @}
  
  size_t stride() const { return stride_; }
  
  void stride( size_t s ) { stride_ = s; }

protected :
  /**
   * Keyword used to declare a kernel.
   * This changed with the version of OpenCL.
   */
  static constexpr char const KERNEL[] = "__kernel";
  
  /**
   * Keyword used to declare global memory.
   * This changed with the version of OpenCL.
   */
  static constexpr char const GLOBAL[] = "__global";

  /**
   * Create a new kernel template.
   *
   */  
  KernelTemplate(
    size_t mwg, size_t nwg, size_t kwg,
    size_t kwi,
    size_t mdimc, size_t ndimc,
    size_t mdima, size_t ndimb, 
    VectorSize v,
    size_t stride, std::string const& nm
  ):
    Mwg_( mwg ), Nwg_( nwg ), Kwg_( kwg ), MdimC_( mdimc ), NdimC_( ndimc ), Kwi_( kwi ), vw_( v ), name_( nm ), MdimA_( mdima ), NdimB_( ndimb ), stride_( stride )
  {
    assert( isValidKernelName( name() ) );
  }
  
private :
  static bool isValidKernelName( const std::string& name )
  {
    // Regex does not work yet.
    // return std::regex_match( name, std::regex( "[_a-zA-Z][_a-zA-Z0-9]*", std::regex_constants::basic ) );
    
    auto const stringIsNotEmpty                    = !name.empty();
    auto const firstCharIsAlphaOrUnderscore        = name[0] == '_' || (name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z');
    auto const allCharsAreAlphanumericOrUnderscore = std::strspn( name.c_str(), "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_" ) == name.length();
    
    return stringIsNotEmpty && firstCharIsAlphaOrUnderscore && allCharsAreAlphanumericOrUnderscore;
  }
  
  size_t Mwg_, Nwg_, Kwg_; ///< outer blocking factors. wg = work group
  size_t MdimC_, NdimC_; ///< Inner blocking factors.
  size_t Kwi_; ///< Unroll factor.
  VectorSize vw_; ///< Dimension of the vector data type used.
  std::string name_; ///< Name of the function that represents the kernels logic.
  size_t MdimA_, NdimB_;
  size_t stride_;
};



template< typename Matrix >
constexpr char const KernelTemplate< Matrix >::KERNEL[];



template< typename Matrix >
constexpr char const KernelTemplate< Matrix >::GLOBAL[];



/**
 * Compute the index into a matrix.
 * 
 * @param x Expression for the column.
 * @param y Expression for the row.
 * @param m Matrix to decide how the underlining memory is accessed.
 * 
 * @return An expression that can be used to access the elements of an array that is a matrix with a specific memory layout.
 * 
 * @pre The inner blocks of the must all have the same size i.e. no blocks are allowed to have less elements especially at the matrix' borders.
 */
template< typename T >
std::string index( std::string const& x, std::string const& y, utl::Matrix2< T, utl::column_major_tag, utl::row_major_tag > const& m )
{
  assert( m.rows() % m.innerRows() == 0 );
  assert( m.cols() % m.innerCols() == 0 );
  
  auto const row = '(' + y + ')';
  auto const col = '(' + x + ')';
  
  auto const innerBlockRow( row + " / " + std::to_string( m.innerRows() ) );
  auto const innerBlockCol( col + " / " + std::to_string( m.innerCols() ) );
  
  // See precondition.
  auto const colsOfBlock( m.innerCols() );
  
  auto const innerElementRow( row + " % " + std::to_string( m.innerRows() ) );
  auto const innerElementCol( col + " % " + std::to_string( m.innerCols() ) );
  
  auto const blockIndex(
    '(' + innerBlockCol + ") * " + std::to_string( m.innerCols() * m.rows() ) + " + (" +
    innerBlockRow + ") * " + std::to_string( m.innerRows() * colsOfBlock )
  );
  
  auto const elementIndex(
    '(' + innerElementRow + ") * " + std::to_string( colsOfBlock ) + " + (" + innerElementCol + ')'
  );
  
  return blockIndex + " + " + elementIndex;
}



/**
 * Compute the index into an array that represents a matrix.
 * 
 * @param x Expression for the column.
 * @param y Expression for the row.
 * @param width Number of matrix columns
 * @param height Number of matrix rows.
 * 
 * @return An expression that can be used to index an array that is a matrix.
 */
inline std::string index( std::string const& x, std::string const& y, size_t width, size_t /* height */ )
{ 
  return "((" + y + ") * " + std::to_string( width ) + " + (" + x + "))";
}

#endif
