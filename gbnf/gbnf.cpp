/*! Program converts EBNF-style grammar file to an easily-parseable GrylloBNF format.
 *  Several options may apply: 
 *  - convert EBNF to .gbnf file
 *  - convert EBNF to C/C++ header with gBNF structures filled in with the gBNF data.
 *  - convert the .gbnf file to C/C++ header defined above.
 */ 

#include "gbnf.h"

namespace gbnf{

void Tools::convertToGbnf(GbnfData& data, std::istream& input){
    
}

void Tools::makeCHeaderFile(const GbnfData& data, const char* variableName, std::ostream& output){

}

}


