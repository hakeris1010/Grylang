#include <iostream>
#include <sstream>
#include <gryltools/execution_time.hpp>
#include "grylloparse.hpp"

// Include source so we can use the implementation.
#include "../lexer.cpp"

/*! Benchmark tests the LexerImpl's Dedicated runner (regex_iterator loop)
 *  vs Standard runner (repeatedly calling one-token getNextTokenPriv).
 *
 *  - We find out that on buffer sizes high enough, the execution time of 
 *    both runners becomes nearly the same.
 *  - at BUFFSIZE = 2048, ITERATIONS = 1000, and the test program as below,
 *    the exectution time of each runner is ~0.8 seconds. 
 *    (So runner's single runtime is ~0.0008 sec.)
 *
 *  - However, with smaller buffer sizes the execution times grow, but not dramatically.
 *    Even at BUFFSIZE = 5, it takes only 2x longer than with BUFFSIZE = 2048.
 *    And, both runners still perform basically the same.
 *
 *  CONCLUSION: it's pointless to use a dedicated runner. Just use the standard
 *    single-token repeated runner. This save much code, and improves flexibility,
 *    when using custom tokenizers.
 */ 

const char* testLexics =
"<ident> := \"\\w+\" ;\n"
"<operator> := \"[;=+\\-\\*/\\[\\]{}<>%]\" ;\n"
;

const size_t ITERATIONS = 1000;
const size_t BUFFSIZE = 2048;

const char* testProgram = 
"aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk"
"aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk"
"aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk"
"aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk"
"aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk"
"aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk"
"aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk"
"aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk"
"aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk"
"aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk"
"aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk"
"aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk"
"aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk"
"aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk"
;

int main(int argc, char** argv){
    // Stringstream for data.
    std::istringstream sstr( testLexics );

    // Test the GBNF stuff
    gbnf::GbnfData lexicData;

    gbnf::convertToGbnf( lexicData, sstr );
    gbnf::convertToBNF( lexicData );
     
    //std::cout<<"\nlexicData:\n"<< lexicData <<"\n\n";

    gparse::RegLexData lexicon( lexicData, true );

    //std::cout<<"\n"<< lexicon <<"\n\n";

    std::cout<<"\n=========================\n\nBenchmarking Dedicated Runner.\n\n";
    std::cout<<"Execution took " <<
    gtools::functionExecTimeRepeated( [&](){
        std::istringstream pstream( testProgram );
        gparse::LexerImpl lexer( lexicon, pstream, true, 0, true, BUFFSIZE ); 
        lexer.start();
    }, ITERATIONS ).count()
    << " seconds\n";

    std::cout<<"\n=========================\n\nBenchmarking Standard Runner.\n\n";
    std::cout<<"Execution took " <<
    gtools::functionExecTimeRepeated( [&](){
        std::istringstream pstream( testProgram );
        gparse::LexerImpl lexer( lexicon, pstream, true, 0, false, BUFFSIZE ); 
        lexer.start();
    }, ITERATIONS ).count()
    << " seconds\n";

    /*if( lexer ){
        gparse::LexicToken tok;
        while( lexer->getNextToken( tok ) ){
            std::cout<< "\nGOT TOKEN!!! : \n"<< tok <<"\n\n";
        }
    }*/

    return 0;
}


