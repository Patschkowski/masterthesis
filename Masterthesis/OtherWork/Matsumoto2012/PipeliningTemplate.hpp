#ifndef PIPELINING_TEMPLATE_HPP
#define PIPELINING_TEMPLATE_HPP

#include "KernelTemplate.hpp"

template< typename Matrix >
class PipeliningTemplate : public KernelTemplate< Matrix >
{
public :
  void generate( std::ostream& os , Matrix const& lhs, Matrix const& rhs, Matrix const& result ) const override
  {
    // Do some sanity checks.
    assert( lhs.cols() == rhs.rows() );
    assert( lhs.innerCols() == rhs.innerRows() );
    assert( lhs.innerRows() == result.innerRows() );
    assert( rhs.innerCols() == result.innerCols() );
    assert( lhs.rows() == result.rows() );
    assert( rhs.cols() == result.cols() );
    
    std::ostringstream oss;
    
    auto const dataType = utl::getType< typename Matrix::value_type >().name();
    
    
    oss << KERNEL << " void " << this->name() << "( " << 
                       dataType << " const alpha, " << 
      GLOBAL << ' ' << dataType << " const* A, " <<
      GLOBAL << ' ' << dataType << " const* B, " <<
                       dataType << " const beta, " <<
      GLOBAL << ' ' << dataType << "* const C )\n{\n";
        
    oss << "  local "   << dataType << " Alm[" << this->Mwg() * this->Kwg() << "], Blm[" << this->Kwg() * this->Nwg() << "];\n" << 
           "  private " << dataType << " Cpm[" << this->Mwi() * this->Nwi() << "] = { 0 };\n\n";
           
    
    oss << "  size_t lx = get_local_id(0), ly = get_local_id(1), gx = get_group_id(0), gy = get_group_id(1);\n\n";
    
    for ( size_t i = 0; i < this->MwiA(); ++i )
      for ( size_t j = 0; j < this->KwiA(); ++j )
      {
        auto const indexX = std::to_string( this->KwiA() ) + " * lx + " + std::to_string( j );
        auto const indexY = std::to_string( this->MwiA() ) + " * ly + " + std::to_string( i );
        
        oss << "  Alm[" << 
          index( indexX, indexY, this->Kwg(), this->Mwg() ) << "] = A[" << 
          index( indexX + " + 0",
                indexY + " + gy * " + std::to_string( this->Mwg() ), lhs ) << "];\n";
      }
                
    for ( size_t i = 0; i < this->KwiB(); ++i )
      for ( size_t j = 0; j < this->NwiB(); ++j )
      {
        auto const indexX = std::to_string( this->NwiB() ) + " * lx + " + std::to_string( j );
        auto const indexY = std::to_string( this->KwiB() ) + " * ly + " + std::to_string( i );
        
        oss << "  Blm[" <<
          index( indexX, indexY, this->Nwg(), this->Kwg() ) << "] = B[" << 
          index( indexX + " + gx * " + std::to_string( this->Nwg() ),
                indexY + " + 0", rhs ) << "];\n";
      }
    
    oss << "\n  barrier( CLK_LOCAL_MEM_FENCE );\n\n";
    
    oss << "  for ( size_t pwg = " << this->Kwg() << "; pwg <= " << lhs.cols() - this->Kwg() << "; pwg += " << this->Kwg() << " )\n  {\n";
      
    oss << "    private " << dataType << " Apm0[" << this->MwiA() * this->KwiA() << "], Bpm0[" << this->KwiB() * this->NwiB() << "];\n\n";
    
    // TODO Load.
    for ( size_t i = 0; i < this->MwiA(); ++i )
      for ( size_t j = 0; j < this->KwiA(); ++j )
      {
        auto const indexX = std::to_string( this->KwiA() ) + " * lx + " + std::to_string( j );
        auto const indexY = std::to_string( this->MwiA() ) + " * ly + " + std::to_string( i );
        
        oss << "    Apm0[" << index( std::to_string( j ), std::to_string( i ), this->KwiA(), this->MwiA() ) << "] = A[" << 
          index( indexX + " + pwg", indexY + " + gy * " + std::to_string( this->Mwg() ), lhs ) << "];\n";
        
        /*oss << "    Alm[" << 
          index( indexX, indexY, this->Kwg(), this->Mwg() ) << "] = A[" << 
          index( indexX + " + pwg",
                indexY + " + gy * " + std::to_string( this->Mwg() ), lhs ) << "];\n";*/
      }
                
    for ( size_t i = 0; i < this->KwiB(); ++i )
      for ( size_t j = 0; j < this->NwiB(); ++j )
      {
        auto const indexX = std::to_string( this->NwiB() ) + " * lx + " + std::to_string( j );
        auto const indexY = std::to_string( this->KwiB() ) + " * ly + " + std::to_string( i );
        
        oss << "    Bpm0[" << index( std::to_string( j ), std::to_string( i ), this->NwiB(), this->KwiB() ) << "] = B[" <<
          index( indexX + " + gx * " + std::to_string( this->Nwg() ), indexY + " + pwg", rhs )<< "];\n";
        
        /*oss << "    Blm[" <<
          index( indexX, indexY, this->Nwg(), this->Kwg() ) << "] = B[" << 
          index( indexX + " + gx * " + std::to_string( this->Nwg() ),
                indexY + " + pwg", rhs ) << "];\n";*/
      }
    // End Load.
    
    oss << "    barrier( CLK_LOCAL_MEM_FENCE );\n";
    
    oss << "    for ( size_t pwi = 0; pwi <= " << this->Kwg() - this->Kwi() << "; pwi += " << this->Kwi() << " )\n    {\n";
      
    oss << "      private " << dataType << " Apm1[" << this->Mwi() * this->Kwi() << "], Bpm1[" << this->Kwi() * this->Nwi() << "];\n\n";
    
    for ( size_t i = 0; i < this->Mwi(); ++i )
      for ( size_t j = 0; j < this->Kwi(); ++j )
        oss << "      Apm1[" << index( std::to_string( j ), std::to_string( i ), this->Kwi(), this->Mwi() ) << "] = Alm[" << 
          index( 
            "pwi + " + std::to_string( j ), 
            std::to_string( this->Mwi() ) + " * ly + " + std::to_string( i ), this->Kwg(), this->Mwg() ) << "];\n";
      
    oss << '\n';
      
    for ( size_t i = 0; i < this->Kwi(); ++i )
      for ( size_t j = 0; j < this->Nwi(); ++j )
        oss << "      Bpm1[" << index( std::to_string( j ), std::to_string( i ), this->Nwi(), this->Kwi() ) << "] = Blm[" << 
          index( 
            std::to_string( this->Nwi() ) + " * lx + " + std::to_string( j ),
            "pwi + " + std::to_string( i ), this->Nwg(), this->Kwg() ) << "];\n";
      
    oss << '\n';
      
    for ( size_t i = 0; i < this->Mwi(); ++i )
      for ( size_t j = 0; j < this->Nwi(); ++j )
        for ( size_t k = 0; k < this->Kwi(); ++k )
          oss << "      Cpm[" << index( std::to_string( j ), std::to_string( i ), this->Nwi(), this->Mwi() ) << "] += Apm1[" << 
            index( std::to_string( k ), std::to_string( i ), this->Kwi(), this->Mwi() ) << "] * Bpm1[" << 
            index( std::to_string( j ), std::to_string( k ), this->Nwi(), this->Kwi() ) << "];\n";
    oss << "    }\n";
    
    oss << "    barrier( CLK_LOCAL_MEM_FENCE );\n";
    
    // TODO Store.
    for ( size_t i = 0; i < this->MwiA(); ++i )
      for ( size_t j = 0; j < this->KwiA(); ++j )
      {
        auto const indexX = std::to_string( this->KwiA() ) + " * lx + " + std::to_string( j );
        auto const indexY = std::to_string( this->MwiA() ) + " * ly + " + std::to_string( i );
        
        oss << "    Alm[" << index( indexX, indexY, this->Kwg(), this->Mwg() ) << "] = Apm0[" << 
          index( std::to_string( j ), std::to_string( i ), this->KwiA(), this->MwiA() ) << "];\n";
        
       /* oss << "    Alm[" << 
          index( indexX, indexY, this->Kwg(), this->Mwg() ) << "] = A[" << 
          index( indexX + " + pwg",
                indexY + " + gy * " + std::to_string( this->Mwg() ), lhs ) << "];\n";*/
      }
                
    for ( size_t i = 0; i < this->KwiB(); ++i )
      for ( size_t j = 0; j < this->NwiB(); ++j )
      {
        auto const indexX = std::to_string( this->NwiB() ) + " * lx + " + std::to_string( j );
        auto const indexY = std::to_string( this->KwiB() ) + " * ly + " + std::to_string( i );
        
        oss << "   Blm[" << index( indexX, indexY, this->Nwg(), this->Kwg() ) << "] = Bpm0[" << 
          index( std::to_string( j ), std::to_string( i ), this->NwiB(), this->KwiB() ) << "];\n";
        
/*        oss << "    Blm[" <<
          index( indexX, indexY, this->Nwg(), this->Kwg() ) << "] = B[" << 
          index( indexX + " + gx * " + std::to_string( this->Nwg() ),
                indexY + " + pwg", rhs ) << "];\n";*/
      }
    // Store End.
      
    oss << "    barrier( CLK_LOCAL_MEM_FENCE );\n";
    
    oss << "  }\n";
    
    oss << "  for ( size_t pwi = 0; pwi <= " << this->Kwg() - this->Kwi() << "; pwi += " << this->Kwi() << " )\n  {\n";
    
    oss << "      private " << dataType << " Apm1[" << this->Mwi() * this->Kwi() << "], Bpm1[" << this->Kwi() * this->Nwi() << "];\n\n";
    
    for ( size_t i = 0; i < this->Mwi(); ++i )
      for ( size_t j = 0; j < this->Kwi(); ++j )
        oss << "      Apm1[" << index( std::to_string( j ), std::to_string( i ), this->Kwi(), this->Mwi() ) << "] = Alm[" << 
          index( 
            "pwi + " + std::to_string( j ), 
            std::to_string( this->Mwi() ) + " * ly + " + std::to_string( i ), this->Kwg(), this->Mwg() ) << "];\n";
      
    oss << '\n';
      
    for ( size_t i = 0; i < this->Kwi(); ++i )
      for ( size_t j = 0; j < this->Nwi(); ++j )
        oss << "      Bpm1[" << index( std::to_string( j ), std::to_string( i ), this->Nwi(), this->Kwi() ) << "] = Blm[" << 
          index( 
            std::to_string( this->Nwi() ) + " * lx + " + std::to_string( j ),
            "pwi + " + std::to_string( i ), this->Nwg(), this->Kwg() ) << "];\n";
      
    oss << '\n';
      
    for ( size_t i = 0; i < this->Mwi(); ++i )
      for ( size_t j = 0; j < this->Nwi(); ++j )
        for ( size_t k = 0; k < this->Kwi(); ++k )
          oss << "      Cpm[" << index( std::to_string( j ), std::to_string( i ), this->Nwi(), this->Mwi() ) << "] += Apm1[" << 
            index( std::to_string( k ), std::to_string( i ), this->Kwi(), this->Mwi() ) << "] * Bpm1[" << 
            index( std::to_string( j ), std::to_string( k ), this->Nwi(), this->Kwi() ) << "];\n";
    
    oss << "  }\n";
    
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
  
  PipeliningTemplate( size_t stride ): 
    PipeliningTemplate( 
                   96, 96, 16,
                   2,
                   16, 16,
                   16, 16,
                   VectorSize::One,
                   stride,
                   "sgemm"
                 )
  { }
  
  PipeliningTemplate( 
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
  using KernelTemplate< Matrix >::KERNEL;
  using KernelTemplate< Matrix >::GLOBAL;
};

#endif
