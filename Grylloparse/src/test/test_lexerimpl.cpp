#include <iostream>
#include <sstream>
#include "grylloparse.hpp"
#include "../lexer.cpp"

const bool useMultithreading = true;
const size_t BUFF_SIZE = 5;

const char* testLexics =
"<ident> := \"\\w+\" ;\n"
"<operator> := \"[;=+\\-\\*/\\[\\]{}<>%]\" ;\n"
;

//const char* testProgram = "aaaaaabbbbbbbbbbb;11";
//"int i = 0;;";

const char* testProgram = 
"aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk"
;

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

    gparse::BaseLexer* lexer = nullptr;
    
    if( useMultithreading ){
        lexer = new gparse::LexerImpl( lexicon, pstream, true, 3, true, BUFF_SIZE );
        lexer->start();
    }
    else
        lexer = new gparse::LexerImpl( lexicon, pstream, false, 3, true, BUFF_SIZE );

    const size_t limit = (size_t)(-1);
    if( lexer ){
        gparse::LexicToken tok;
        size_t i = 0;
        while( lexer->getNextToken( tok ) && (i++ < limit) ){
            std::cout<< "\nGOT TOKEN!!! : \n"<< tok <<"\n\n";
        }

        delete lexer;
    }

    return 0;
}

