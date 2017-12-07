#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cctype>
#include <gryltools/stackreader.hpp>
#include "gbnf.hpp"
extern "C" {
    #include <gryltools/hlog.h>
}

namespace gbnf{

/*===========================================================//
 * C++ Header file generator.
 * - Works by taking a GbnfData variable, and outputting it's construction
 *   code in C++ format.
 */   
class GbnfCodeGenerator
{
private:
    // Core:
    std::ostream& output;
    const GbnfData& data;

    std::string variableName;
    std::string includeGuard;

    // Helper Methods
    void makeStringProperties( const std::string& varName ); 
    void outputTagTable();
    void outputGrammarTable();

public:
    /*! Constructor. Just makes sure all necessary data is set checked.
     */  
    GbnfCodeGenerator(const GbnfData& gbData, std::ostream& outp, const std::string& varName)
        : output( outp ), data( gbData )
    { 
        makeStringProperties( varName ); 
    }
    
    void generate();
};
   
/*! Sets the proper variable names and other string properties.
 */ 
void GbnfCodeGenerator::makeStringProperties( const std::string& varNameStart ){
    variableName = varNameStart;
    std::string& vn = variableName;

    // First char must be letter.
    while( !vn.empty() && !(std::isalpha( vn[0] ) || vn[0]=='_') )
        vn.erase(0, 1);

    if(vn.empty())
        vn.assign("yourGbnfData");
    else{
        for( auto &a : vn ){
            // Replace all invalid characters with '_'.
            if( !std::isalnum( a ) && a != '_' )
                a = '_';
        }
    }

    // Variable name is done. Now generate include guard.
    includeGuard = variableName;
    std::transform( includeGuard.begin(), includeGuard.end(), includeGuard.begin(), ::toupper);
    includeGuard.append("_HPP_INCLUDED");

    std::cout<<"VarName: "<<variableName<<", incGuard: "<<includeGuard<<"\n";
}


/*! Actually generates the code and outputs it.
 */ 
void GbnfCodeGenerator::generate(){
    output << "\n#ifndef "<< includeGuard <<"\n#define "<< includeGuard <<"\n\n";
    output << "/* File automatically generated by GBNFCodeGen Tool.\n";
    output << " * Edit at your own risk.\n */\n\n";
    output << "#include <gbnf.hpp>\n\n";

    output << "const GbnfData "<< variableName<< "= GbnfData( "<< data.flags <<" , \n";
    
    // Output TagTbl constructor
    outputTagTable();

    // Output GrammarTbl constructor
    outputGrammarTable();

    //GbnfData nuda( 1, { NonTerminal(1, "kaka"), NonTerminal(2, "baba"), }, {} ); 
    //output<< nuda;
}

void GbnfCodeGenerator::outputTagTable(){

}

void GbnfCodeGenerator::outputGrammarTable(){

}


//==========================================================//

/*! GBNF TOOLS.
 *  Generates a C++ header file from GBNF data passed, and outputs to a stream.
 */ 
void generateCode(const GbnfData& data, std::ostream& output, const char* variableName){
    GbnfCodeGenerator gen( data, output, std::string( variableName ) );
    gen.generate();
}

} // Namespace gbnf end.

//end.
