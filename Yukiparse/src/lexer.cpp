#include "lexer.hpp"
#include <gryltools/blockingqueue.hpp>
#include <gryltools/stackreader.hpp>

namespace yparse{

class LexerImpl : public BaseLexer
{
private:
    const bool useBlockingQueue;
    gbnf::GbnfData lexics;

    // For token extraction we'll use GrylTools's StreamReader class.
    gtools::StackReader rdr;

    // We'll use this only if useBlockingQueue.
    std::unique_ptr< gtools::BlockingQueue< LexicToken > > bQueue;
     
    // State variables
    volatile bool running = false; 
    volatile bool endOfStream = false;
    size_t lineCount = 0;
    size_t posInLine = 0;

    // Backend function for getting next token.
    int getNextTokenPriv( LexicToken& tok );
    void throwError( std::string message );

public:
    LexerImpl( const gbnf::GbnfData& lexicData, std::istream& stream, bool useBQ = false )
        : useBlockingQueue( useBQ ), lexics( lexicData ), rdr( stream ), 
          bQueue( useBQ ? new gtools::BlockingQueue< LexicToken >() : nullptr )
    {}

    LexerImpl( gbnf::GbnfData&& lexicData, std::istream& stream, bool useBQ = false )
        : useBlockingQueue( useBQ ), lexics( std::move(lexicData) ), rdr( stream ), 
          bQueue( useBQ ? new gtools::BlockingQueue< LexicToken >() : nullptr )
    {}

    void start();
    bool getNextToken( LexicToken& tok );
};

/*! A little helper for throwing errors.
 */ 
void LexerImpl::throwError( std::string message ){
    throw std::runtime_error( "[" + std::to_string( lineCount ) +":"+
                             std::to_string( posInLine )+"]: "+message );
}

/*! Main backend function.
 *  Does all the true job of getting the tokens.
 */ 
int LexerImpl::getNextTokenPriv( LexicToken& tok ){
    tok.id = 1337;
    tok.data = "kawaii~~";

    endOfStream = true;
    return 1;
}

/*! Runner function which fills the blocking queue with tokens.
 *  - Runs until the end of stream, or until error occurs.
 *  - CALLING RULES:
 *    - Can be called only when multithreading (using the Blocking Queue).
 *    - Also, only one instance of this function may execute at time.
 */ 
void LexerImpl::start(){
    // Check if call conditions are met (not running already, and using threads).
    if( running || !useBlockingQueue )
        return;
    running = true;

    while( 1 ){
        // Allocate a new token.
        LexicToken tok;
        int tt = getNextTokenPriv( tok );

        // If good, put to queue.
        if(tt == 0)
            bQueue->push( std::move( tok ) );

        // TODO: check for non-fatal errors, and just ignore non-fatal tokens.
        // Break loop if error or loop end.
        break;
    }

    running = false;    
}

/*! Get next token. 
 *  - If no multithreading is used, it directly calls the backend function.
 *  - Can return invalid tokens if non-fatal error occured.
 *  @param tok - reference to token to-be-filled.
 *  @return true if more tokens can be expected - not end.
 */ 
bool LexerImpl::getNextToken( LexicToken& tok ){
    if( endOfStream )
        return false;

    // If using multithreading, wait until token gets put into the queue.
    if( useBlockingQueue ){
        tok = bQueue->pop();
        return true;
    }
    
    // If no multithreading, just directly call the private token getter.
    int ret = getNextTokenPriv( tok );

    // return true (have more tokens) if normal token or non-fatal error.
    return ret >= 0;
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

