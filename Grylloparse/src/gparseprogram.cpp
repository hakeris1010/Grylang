#include <iostream>
#include <sstream>
#include "grylloparse.hpp"
#include "lexer.hpp"

const bool useMultithreading = true;

const char* testLexics =
"<ident> := \"\\w+\" ;\n"
"<operator> := \"[;=+\\-\\*/\\[\\]{}<>%]\" ;\n"
;
//"<ignore> := \"[abc]\" ;"
//"<regex_delim> := <operator> | \"\\w\" ;"
;

const char* testProgram = "aaaaaabbbbbbbbbbb;11";
//"int i = 0;;";


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

    gparse::RegLexData lexicon( lexicData, true );

    std::cout<<"\n"<< lexicon <<"\n\n";
    std::cout<<"=========================\n\nTokenizing by Lexics...\n\n";

    std::istringstream pstream( testProgram );

    gparse::Lexer* lexer = nullptr;
    
    if( useMultithreading ){
        lexer = new gparse::Lexer( lexicon, pstream, true );
        lexer->start();
    }
    else
        lexer = new gparse::Lexer( lexicon, pstream );

    if( lexer ){
        gparse::LexicToken tok;
        while( lexer->getNextToken( tok ) ){
            std::cout<< "\nGOT TOKEN!!! : \n"<< tok <<"\n\n";
        }

        delete lexer;
    }

    return 0;
}

