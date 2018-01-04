#include <iostream>
#include <sstream>
#include "grylloparse.hpp"
#include "lexer.hpp"

// Test Lexics.
const char* testLexics =
"<ident> := \"\\w+\" ;\n"
"<operator> := \"[;=+\\-\\*/\\[\\]{}<>%]\" ;\n"
"<delim> := <operator> | \"\\w\" ;"
;

const char* testProgram =
"int i = 0;\nwhile(i)=0;";


int main(int argc, char** argv){
    std::cout<<"Newt newt!\n";
    
    // Stringstream for data.
    std::istringstream sstr( testLexics );

    // Test the GBNF stuff
    gbnf::GbnfData lexicData;

    std::cout<<"Convering lexic data to GBNF...\n";

    gbnf::convertToGbnf( lexicData, sstr );
    gbnf::convertToBNF( lexicData );
     
    std::cout<<"\nlexicData:\n"<< lexicData <<"\n\n";

    gparse::RegLexData lexicon( lexicData );

    std::cout<<"\nRegLexData:\n"<< lexicon <<"\n\n";
    std::cout<<"=========================\n\nTokenizing by Lexics...\n\n";

    std::istringstream pstream( testProgram );

    gparse::Lexer lexer( lexicon, pstream );

    bool areMore = true;
    while( areMore ){
        gparse::LexicToken tok;
        areMore = lexer.getNextToken( tok );

        if(areMore)
            std::cout<< tok <<"\n";
    }

    return 0;
}

