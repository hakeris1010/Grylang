#include "lexer.hpp"
#include <gryltools/blockingqueue.hpp>

namespace yparse{

class LexerImpl : public BaseLexer
{
private:
    // Main variables
    const bool useBlockingQueue;
    volatile bool running = false;

    gbnf::GbnfData lexics;

    std::istream& is;

    // We'll use this only if useBlockingQueue.
    std::unique_ptr< gtools::BlockingQueue< LexicToken > > bQueue;

    // Backend function for getting next token.
    int getNextTokenPriv( LexicToken& tok );
    void ThrowError

public:
    LexerImpl( const gbnf::GbnfData& lexicData, std::istream& stream, bool useBQ = false )
        : useBlockingQueue( useBQ ), lexics( lexicData ), is( stream ), 
          bQueue( useBQ ? new gtools::BlockingQueue< LexicToken >() : nullptr )
    {}

    LexerImpl( gbnf::GbnfData&& lexicData, std::istream& stream, bool useBQ = false )
        : useBlockingQueue( useBQ ), lexics( std::move(lexicData) ), is( stream ), 
          bQueue( useBQ ? new gtools::BlockingQueue< LexicToken >() : nullptr )
    {}

    void start();
    bool getNextToken( LexicToken& tok );
};

void LexerImpl::start(){
    if( running )
        return;
    running = true;

    while( 1 ){
        LexicToken tok;
        int tt = getNextTokenPriv( tok );
        if(tt==0){
            
        }
        else

    }

    running = false;    
}

bool LexerImpl::getNextToken( LexicToken& tok ){
    return true;
}

/*=============================================================
 * Public Lexer methods.
 */ 
Lexer::Lexer( const gbnf::GbnfData& lexicData, std::istream& stream, bool useBQ )
    : impl( new LexerImpl( lexicData, stream, useBQ ) )
{}

Lexer::Lexer( gbnf::GbnfData&& lexicData, std::istream& stream, bool useBQ )
    : impl( new LexerImpl( lexicData, stream, useBQ ) )
{}

void Lexer::start(){
    impl->start();
}

bool Lexer::getNextToken( LexicToken& tok ){
    return impl->getNextToken( tok );
}

}

