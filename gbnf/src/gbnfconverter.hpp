#ifndef GBNF_CONVERTER_HPP_INCLUDED
#define GBNF_CONVERTER_HPP_INCLUDED

#include "gbnf.hpp"

namespace gbnf{

/*! A tool for manipulating the (e)BNF language in GBNF format.
 *  Can be used for:
 *  - Removing left/right recursion
 *  - Converting eBNF to simple BNF, for easier parsing.
 */ 
namespace Converter{
    void convertToSimpleBNF( GbnfData& data );
    void removeLeftRecursion( GbnfData& data );
    void fullyFixGBNF( GbnfData& data );
};

}

#endif // GBNF_CONVERTER_HPP_INCLUDED

