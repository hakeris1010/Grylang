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
 *  Contains information about line positions, etc.
 */
struct StreamStats{
    size_t lineCount = 0;
    size_t posInLine = 0;

    StreamStats(){}
    StreamStats(size_t lns, size_t posl) : lineCount( lns ), posInLine( posl ) {}
};

/*! Main lexer implementation.
 *  - Takes RegLexData as a lexicon grammar, passed as ctor parameter.
 *  - Assumes that the passed RegLexData is fully valid. No checks are being done.
 *  - Tokenizes the stream by using rules provided in the RegLexData passed.
 */ 
class LexerImpl : public BaseLexer
{
// Specific constants.
public:
    const static size_t BUFFER_SIZE = 2048;

    // Token extractor responses.
    // < 0 means fatal error
    // = 0 means good, normal token
    // > 0 means unusual token
    const static int END_OF_FILE = -1;
    const static int GOOD_TOKEN = 0;

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
    std::function< bool(char) > isWhitespace( [](char c){ return std::iswspace( c ) } );
     
    // State variables
    volatile bool running = false; 
    volatile bool endOfStream = false;

    // Stream state specifics.
    StreamStats stats;    

    // Token's store buffer.
    char buffer[ BUFFER_SIZE ];
    char* bufferPointer;
    const char* bufferEnd;

    // Backend functions.
    void throwError( std::string message );
    inline void updateLineStats( char c );

    bool updateBuffer();
    int getNextTokenPriv( LexicToken& tok );

    void setCustomWhitespacing(){
        if( lexicData.useCustomWhitespaces && !lexicData.ignorables.empty() ){
            isWhitespace = [](char c){ 
                return (lexicData.ignorables.find( c ) != std::string::npos);
            };
        }
    }

public:
    // Move-semantic and Copy-semantic constructors.
    LexerImpl( const RegLexData& lexicData, std::istream& stream, bool useBQ = false )
        : useBlockingQueue( useBQ ), lexics( lexicData ), rdr( stream ), 
          bQueue( useBQ ? new gtools::BlockingQueue< LexicToken >() : nullptr )
    { setCustomWhitespacing(); }

    LexerImpl( RegLexData&& lexicData, std::istream& stream, bool useBQ = false )
        : useBlockingQueue( useBQ ), lexics( std::move(lexicData) ), rdr( stream ), 
          bQueue( useBQ ? new gtools::BlockingQueue< LexicToken >() : nullptr )
    { setCustomWhitespacing(); }

    void start();
    bool getNextToken( LexicToken& tok );
};

/*! A little helper for throwing errors.
 */ 
void LexerImpl::throwError( std::string message ){
    throw std::runtime_error( "[" + std::to_string( stats.lineCount ) +":"+
                             std::to_string( stats.posInLine )+"]: "+message );
}

/*! Updates the read buffer, and sets stream end flag, if ended.
 *  @return true, if read some data, false if no data was read.
 */ 
bool LexerImpl::updateBuffer(){
    // If buffer is already exhausted, read stuff.
    if( bufferPointer >= bufferEnd ){
        rdr.read( buffer, sizeof(buffer) );
        size_t count = rdr.gcount();

        // No chars were read - stream has ended. No more data can be got.
        if( count == 0 )
            return false;

        // Set buffer end to the point where the last read character is.
        bufferEnd = buffer + rdr.gcount();
        bufferPointer = buffer;
    }
    // Buffer is not exhausted - data still avairabru.
    return true;
}

/*! Inline f-on updating stream stats based on character got.
 */ 
inline void LexerImpl::updateLineStats( char c ){
    if( c == '\n' ){
        stats.lineCount++;
        stats.posInLine = 0;
    }
    else
        stats.posInLine++;
}

/*! Main backend function.
 *  Does all the true job of getting the tokens.
 */ 
int LexerImpl::getNextTokenPriv( LexicToken& tok ){
    tok.id = 1337;
    tok.data = "kawaii~~";

    endOfStream = true;
    return 1;

    // Change remaining buffer, and if empty, update it.
    if( !updateBuffer() ){
        // Update buffer returned false - the file has ended. Return END_OF_FILE.
        return END_OF_FILE;
    }

    // Iterate the buffer, searching for the delimiters, and at the same time 
    // update the statistics (line count, etc).

    // Delimiter is just a char array (not a regex) - read a chunk until delimiter.
    if( !lexics.useRegexDelimiters ){
        const char* tokenStart = bufferPointer;
        
        for( ; bufferPointer < bufferEnd; bufferPointer++ ){
            updateLineStats( *bufferPointer );

            // Check if delimiter. Ignorable(Whitespace) is ALWAYS a delimiter.
            if( nonRegexDelimiters.find( *bufferPointer ) != std::string::npos ){
                bool ignore = isWhitespace( *bufferPointer );
                if( tokenStart < bufferPointer ){

                } 

                // Perform regex-search on the current token.
            }
            else
                bufferPointer++;
        }
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

