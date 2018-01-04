#include "lexer.hpp"
#include <gryltools/blockingqueue.hpp>
#include <gryltools/stackreader.hpp>

namespace gparse{
/*
 *  Supported Lexics:
 *  - Must contain a <delim> rule, which defines the delimiters to use when tokenizing.
 *  - The lexem-defining rules must be 1-option only, and the only 
 *    option token must be of REGEX_STRING type.
 *  - Exception is a delimiter - it can contain options referencing to other types.  
 *
 *  TODO: Tokenizable and Non-Tokenizable language support.
 *      - If <delim> tag is found, treat language as tokenizeable, and use ReGeX 
 *        to parse tokens.
 *      - However, if no <delim> is found, treat language as Non-Tokenizable. 
 *        Then ReGeX will be impossible to use, so we'll have these options:
 *        1. Just pass individual chars to a parser
 *        2. Parse tokens with a parsing algorithm (lexer will use yet another Parser).
 *        3. Use a unified Lexer-Parser (related to #2).
 */

/*! Stream state structure.
 *  Contains information about 
 */

/*! Main lexer implementation.
 *  - Takes RegLexData as a lexicon grammar, passed as ctor parameter.
 *  - Assumes that the passed RegLexData is fully valid. No checks are being done.
 *  - Tokenizes the stream by using rules provided in the RegLexData passed.
 */ 
class LexerImpl : public BaseLexer
{
private:
    // Mode and Lexic Data
    const bool useBlockingQueue;
    const RegLexData lexics;

    // NO: For token extraction we'll use GrylTools's StreamReader class.
    //gtools::StackReader rdr;
    
    std::istream& rdr;

    // We'll use this only if useBlockingQueue.
    std::unique_ptr< gtools::BlockingQueue< LexicToken > > bQueue;

    // Tokenizing parameters
    std::string delimiters;
    bool regexedDelimiters = false; // If false, delimiters are just an array of symbols.
     
    // State variables
    volatile bool running = false; 
    volatile bool endOfStream = false;

    // Stream state specifics.
    size_t lineCount = 0;
    size_t posInLine = 0;

    // Token's store buffer.
    std::string tokenBuff;

    // Backend functions.
    int getNextTokenPriv( LexicToken& tok );
    void collectRegexStringFromGTokens( std::string& str, const auto& rule, int recLevel = 0 );
    void throwError( std::string message );

public:
    LexerImpl( const RegLexData& lexicData, std::istream& stream, bool useBQ = false )
        : useBlockingQueue( useBQ ), lexics( lexicData ), rdr( stream ), 
          bQueue( useBQ ? new gtools::BlockingQueue< LexicToken >() : nullptr )
    { tokenBuff.reserve(256); }

    LexerImpl( RegLexData&& lexicData, std::istream& stream, bool useBQ = false )
        : useBlockingQueue( useBQ ), lexics( std::move(lexicData) ), rdr( stream ), 
          bQueue( useBQ ? new gtools::BlockingQueue< LexicToken >() : nullptr )
    { tokenBuff.reserve(256); }

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

    // Delimiter is just a char array (not a regex) - read a chunk until delimiter.
    if( !lexics.useRegexDelimiters ){
        
    }


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

        // Break loop if error or loop end.
        if( tt < 0 || endOfStream )
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
    // If using multithreading, wait until token gets put into the queue.
    // If stream has ended, but queue has tokens, extract them.
    if( useBlockingQueue ){
        if(!endOfStream || !bQueue->isEmpty())
            tok = bQueue->pop();
        return !endOfStream || !bQueue->isEmpty();
    }
    
    // If no multithreading, just directly call the private token getter.
    int ret = getNextTokenPriv( tok );

    // return true (have more tokens) if normal token or non-fatal error.
    return ret >= 0 && !endOfStream;
}

/*=============================================================
 * Public Lexer methods.
 */ 
Lexer::Lexer( const RegLexData& lexicData, std::istream& stream, bool useBQ )
    : impl( new LexerImpl( lexicData, stream, useBQ ) )
{}

Lexer::Lexer( RegLexData&& lexicData, std::istream& stream, bool useBQ )
    : impl( new LexerImpl( lexicData, stream, useBQ ) )
{}

void Lexer::start(){
    impl->start();
}

bool Lexer::getNextToken( LexicToken& tok ){
    return impl->getNextToken( tok );
}

}

