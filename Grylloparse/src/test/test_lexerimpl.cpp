#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>
#include "grylloparse.hpp"
#include "../lexer.cpp"

/*! Unit Tests for LexerImpl.
 *  Uses self-made embedded testing framework.
 */ 

const int verbosity = 0;

const size_t BUFF_SIZE = 5;
const bool   USE_MULTITHREADING   = true;
const bool   USE_DEDICATED_RUNNER = false;

struct LexerTest{
    const std::string lexics;
    const std::string program;
    const std::vector< std::string > tokens;
    const std::vector< int > ids;

    const size_t buffSize;
    const bool useMultithreading;
    const bool useDedicatedRunner;

    LexerTest( std::string&& _lexics, std::string&& _program, 
               std::initializer_list< std::string >&& _tokens,
               std::initializer_list< int >&& _ids = {},
               size_t _buffSize = BUFF_SIZE, 
               bool _useMultithreading = USE_MULTITHREADING, 
               bool _useDedicatedRunner = USE_DEDICATED_RUNNER )
        : lexics( std::move( _lexics ) ), program( std::move( _program ) ), 
          tokens( std::move( _tokens ) ), ids( std::move( _ids ) ), 
          buffSize( _buffSize ), 
          useMultithreading( _useMultithreading ), 
          useDedicatedRunner( _useDedicatedRunner )
    {}

    void print( std::ostream& os ) const {
        os << "LexerTest:\n buffSize: "  << buffSize 
           << "\n useMultithreading: " << useMultithreading
           << "\n useDedicatedRunner: "<< useDedicatedRunner
           << "\n";  
    }
};

inline std::ostream& operator<< ( std::ostream& os, const LexerTest& test ){
    test.print( os );
    return os;
}

const std::vector< LexerTest > tests({
    LexerTest( 
    // Lexics
        "<ident> := \"\\w+\" ;\n" \
        "<operator> := \"[;=+\\-\\*/\\[\\]{}<>%]\" ;\n",
    // Data
        "aaaaaabbbbbbbbbbb;11;babababa;+++++++++ahuibd\n afjba  12 bajbsdjk",
    // Tokens
        { "aaaaaabbbbbbbbbbb",";","11",";","babababa",";","+","+","+",
          "+","+","+","+","+","+","ahuibd", "afjba", "12", "bajbsdjk" },
    // Token IDs.
        { 1, 2, 1, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1 }
    ),

    LexerTest( 
    // Lexics
        "<ident> := \"[abc]+\" ;\n" \
        "<operator> := \"[+\\-]\" ;\n" \
        "<number> := \"\\d+\" ;\n", 
    // Data
        "a+2--  ccacb +",
    // Tokens
        { "a", "+", "2", "-", "-", "ccacb", "+" },
    // Token IDs.
        { 1, 2, 3, 2, 2, 1, 2 },
    // Specifications
        4,
        true
    ),

    LexerTest( 
    // Lexics
        "<ident> := \"[abc]+\" ;\n" \
        "<operator> := \"[+\\-]\" ;\n" \
        "<number> := \"\\d+\" ;\n", 
    // Data
        "a+2-- go",
    // Tokens
        { "a", "+", "2", "-", "-" },
    // Token IDs.
        { 1, 2, 3, 2, 2, -1 }
    ),     

    LexerTest( 
    // Lexics
        "<ident> := \"[abc]+\" ;\n" \
        "<operator> := \"[+\\-]\" ;\n" \
        "<number> := \"\\d+\" ;\n", 
    // Data
        "a+2-- go",
    // Tokens
        { "a", "+", "2", "-", "-" },
    // Token IDs.
        { 1, 2, 3, 2, 2, -1 },
        BUFF_SIZE,
        false
    )
});

int main(int argc, char** argv){
    std::cout<<"[ Testing gparse::LexerImpl ] ... ";
    if( verbosity > 0 )
        std::cout<<"\n\n";

    for( auto&& test : tests ){
        if( verbosity > 0 ){
            std::cout << "\n==================================\n\n" << test << "\n";
        }

        // Stringstream for data.
        std::istringstream sstr( test.lexics );

        // Test the GBNF stuff
        gbnf::GbnfData lexicData;
        gbnf::convertToGbnf( lexicData, sstr );
        gbnf::convertToBNF( lexicData );

        gparse::RegLexData lexicon( lexicData, true );

        std::istringstream pstream( test.program );

        // Create a lexer object.
        gparse::LexerImpl lexer( lexicon, pstream, test.useMultithreading, 
                verbosity - 1, test.useDedicatedRunner, test.buffSize );
        
        // Gather tokens to queue. 
        bool multiError = false;
        try{
            if( test.useMultithreading )
                lexer.start();
        } catch( const std::exception& e ) { 
            if( verbosity > 0 )
                std::cout << "\nERROR OCCURED while Harvesting Tokens in the Runner: " \
                          << e.what() <<"\n\n";
            multiError = true;
        }

        gparse::LexicToken tok;
        size_t i;
        for( i = 0; i < test.tokens.size(); i++ ){
            try{ 
                if( !lexer.getNextToken( tok ) )
                    break;
            } catch( const std::exception& e ){
                if( verbosity > 0 ){
                    std::cout << "\n[ "<< i <<" ] ERROR: " << e.what() << "\n";
                    if( i < test.ids.size() )
                        std::cout <<"test.ids[i]: "<< test.ids[i];
                    std::cout << "\n\n";
                }

                // If error should occur there, the ID should be -1
                if( i < test.ids.size() ){
                    assert( test.ids[i] == -1 );
                    if( i == test.ids.size()-1 )
                        break;
                }
            }

            if( verbosity > 0 )
                std::cout<< "\n[ "<< i <<" ] GOT TOKEN!!! : \n"<< tok <<"\n\n";
             
            // Check if got the same token as we should get.
            assert( tok.data == test.tokens[ i ] );

            // ID checking is optional.
            if( i < test.ids.size() )
                assert( tok.id == test.ids[ i ] );
        }

        // If multithreaded error occured here, we just get tokens until the error.
        if( multiError && i < test.ids.size() ){
            assert( test.ids[i] == -1 );
        }
    }

    if( verbosity > 0 )
        std::cout<<"\n";
    std::cout<<"[ Success! ]\n";

    return 0;
}

