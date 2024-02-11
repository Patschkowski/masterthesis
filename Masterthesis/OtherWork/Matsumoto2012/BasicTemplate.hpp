#ifndef BASIC_TEMPLATE_HPP
#define BASIC_TEMPLATE_HPP

#include "KernelTemplate.hpp"

template< typename Matrix >
class BasicTemplate : public KernelTemplate< Matrix >
{
public :
  void generate( std::ostream& os, Matrix const& lhs, Matrix const& rhs, Matrix const& result ) const override
  {
    // Do some sanity checks.
    assert( lhs.cols() == rhs.rows() );
    assert( lhs.innerCols() == rhs.innerRows() );
    assert( lhs.innerRows() == result.innerRows() );
    assert( rhs.innerCols() == result.innerCols() );
    assert( lhs.rows() == result.rows() );
    assert( rhs.cols() == result.cols() );
    
    // Use a second stream internally for better exception safety.
    std::ostringstream oss;
    
    // Extract the keyword corresponding to the value type of the matrix.
    auto const dataType = utl::getType< typename Matrix::value_type >().name();
    
    // Write the kernel's interface.
    oss << KERNEL << " void " << this->name() << "( " << 
                       dataType << " const alpha, " << 
      GLOBAL << ' ' << dataType << " const* A, " <<
      GLOBAL << ' ' << dataType << " const* B, " <<
                       dataType << " const beta, " <<
      GLOBAL << ' ' << dataType << "* const C )\n{\n";
                
    // Declare two blocks of local memory for subblocks of the matrices A and B. Also define a subbblock of the result matrix C.
    oss << "  local "   << dataType << " Alm[" << this->Mwg() * this->Kwg() << "], Blm[" << this->Kwg() * this->Nwg() << "];\n" << 
           "  private " << dataType << " Cpm[" << this->Mwi() * this->Nwi() << "] = { 0 };\n\n";
   
    // Get relevant information for this kernel instance.
    oss << "  size_t lx = get_local_id( 0 ), ly = get_local_id( 1 ), gx = get_group_id( 0 ), gy = get_group_id( 1 );\n";
          
    // Loop over all lhs and rhs blocks.
    oss << "  for ( size_t pwg = 0; pwg <= " << lhs.cols() - this->Kwg() << "; pwg += " << this->Kwg() << " )\n  {\n";
    
    // Load subblocks of A and B collectively.
    for ( size_t i = 0; i < this->MwiA(); ++i )
      for ( size_t j = 0; j < this->KwiA(); ++j )
      {
        auto const indexX = std::to_string( this->KwiA() ) + " * lx + " + std::to_string( j );
        auto const indexY = std::to_string( this->MwiA() ) + " * ly + " + std::to_string( i );
        
        oss << "    Alm[" << 
          index( indexX, indexY, this->Kwg(), this->Mwg() ) << "] = A[" << 
          index( indexX + " + pwg",
                indexY + " + gy * " + std::to_string( this->Mwg() ), lhs ) << "];\n";
      }
                
    for ( size_t i = 0; i < this->KwiB(); ++i )
      for ( size_t j = 0; j < this->NwiB(); ++j )
      {
        auto const indexX = std::to_string( this->NwiB() ) + " * lx + " + std::to_string( j );
        auto const indexY = std::to_string( this->KwiB() ) + " * ly + " + std::to_string( i );
        
        oss << "    Blm[" <<
          index( indexX, indexY, this->Nwg(), this->Kwg() ) << "] = B[" << 
          index( indexX + " + gx * " + std::to_string( this->Nwg() ),
                indexY + " + pwg", rhs ) << "];\n";
      }
        
    // Wait for all wavefronts to finish.
    oss << "\n    barrier( CLK_LOCAL_MEM_FENCE );\n\n";
        
    // Loop over subblocks of the subblock.
    oss << "    for ( size_t pwi = 0; pwi <= " << this->Kwg() - this->Kwi() << "; pwi += " << this->Kwi() << " )\n    {\n";
      
    oss << "      private " << dataType << " Apm[" << this->Mwi() * this->Kwi() << "], Bpm[" << this->Kwi() * this->Nwi() << "];\n\n";
    
    for ( size_t i = 0; i < this->Mwi(); ++i )
      for ( size_t j = 0; j < this->Kwi(); ++j )
        oss << "      Apm[" << index( std::to_string( j ), std::to_string( i ), this->Kwi(), this->Mwi() ) << "] = Alm[" << 
          index( 
            "pwi + " + std::to_string( j ), 
            std::to_string( this->Mwi() ) + " * ly + " + std::to_string( i ), this->Kwg(), this->Mwg() ) << "];\n";
      
    oss << '\n';
      
    for ( size_t i = 0; i < this->Kwi(); ++i )
      for ( size_t j = 0; j < this->Nwi(); ++j )
        oss << "      Bpm[" << index( std::to_string( j ), std::to_string( i ), this->Nwi(), this->Kwi() ) << "] = Blm[" << 
          index( 
            std::to_string( this->Nwi() ) + " * lx + " + std::to_string( j ),
            "pwi + " + std::to_string( i ), this->Nwg(), this->Kwg() ) << "];\n";
      
    oss << '\n';
      
    // Carry out one matrix multiplication of a sub-subblock.
    for ( size_t i = 0; i < this->Mwi(); ++i )
      for ( size_t j = 0; j < this->Nwi(); ++j )
        for ( size_t k = 0; k < this->Kwi(); ++k )
          oss << "      Cpm[" << index( std::to_string( j ), std::to_string( i ), this->Nwi(), this->Mwi() ) << "] += Apm[" << 
            index( std::to_string( k ), std::to_string( i ), this->Kwi(), this->Mwi() ) << "] * Bpm[" << 
            index( std::to_string( j ), std::to_string( k ), this->Nwi(), this->Kwi() ) << "];\n";
      
    oss << "    }\n\n";
    oss << "    barrier( CLK_LOCAL_MEM_FENCE );\n\n";
    oss << "  }\n\n";
    
    // Merge the results using the other parameters.
    for ( size_t i = 0; i < this->Mwi(); ++i )
      for ( size_t j = 0; j < this->Nwi(); ++j )
      {        
        auto idx = index( 
          "gx * " + std::to_string( this->Mwg() ) + " + lx * " + std::to_string( this->Mwi() ) + " + " + std::to_string( i ), 
          "gy * " + std::to_string( this->Nwg() ) + " + ly * " + std::to_string( this->Nwi() ) + " + " + std::to_string( j ),
          result );
        
        oss << "  C[" << idx << "] = alpha * Cpm[" << index( std::to_string( i ), std::to_string( j ), this->Nwi(), this->Mwi() ) << "] + beta * C[" << idx << "];\n";
      }
      
    oss << "}\n";
    
    // Have better exception safety.
    os << oss.str();
  }
  
  // Implements the best solution mentioned in the paper for Tahiti using SGEMM.
  BasicTemplate( size_t stride ): 
    BasicTemplate( 
                   96, 96, 16,
                   2,
                   16, 16,
                   16, 16,
                   VectorSize::One,
                   stride,
                   "sgemm"
                 )
  { }
  
  BasicTemplate( 
    size_t Mwg,   size_t Nwg, size_t Kwg,
    size_t Kwi,
    size_t MdimC, size_t NdimC,
    size_t MdimA, size_t NdimB,
    VectorSize vw,
    size_t stride,
    std::string const& name
  ): KernelTemplate<Matrix>( Mwg, Nwg, Kwg, Kwi, MdimC, NdimC, MdimA, NdimB, vw, stride, name )
  { }
  
private :
  using KernelTemplate<Matrix>::KERNEL;
  using KernelTemplate<Matrix>::GLOBAL;
};

#endif
