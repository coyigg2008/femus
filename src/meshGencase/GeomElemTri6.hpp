#ifndef __femus_meshGencase_GeomElemTri6_hpp__
#define __femus_meshGencase_GeomElemTri6_hpp__

//Class for
#include <cstdlib>
#include <iostream>

#include "GeomElemBase.hpp"


namespace femus {



class GeomElemTri6 : public GeomElemBase  {

public:
  
     GeomElemTri6();
     
    ~GeomElemTri6();
  
    unsigned int  get_dimension() const { return 2; };
    unsigned int n_nodes()        const { return 6; };
    std::string   get_name_med()  const { return "TR6"; };
    std::string   get_name_xdmf() const { return "Triangle_6"; };
    
      float get_embedding_matrix(const uint,const uint,const uint);


      double get_prol(const uint /*j*/) {std::cout << "Tri6: no prolongation needed\n"; abort();};

private:
    
      static const float _embedding_matrix[4][6][6];   // (volume)
      
};


} //end namespace femus



#endif
