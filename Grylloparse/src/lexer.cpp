#include "lexer.hpp"
#include <functional>
#include <iostream>
#include <gryltools/blockingqueue.hpp>

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
class LexerImpl_SimpleDelim : public BaseLexer
{
// Specific constants.
public:
    const static size_t BUFFER_SIZE = 2048;

    // Token extractor responses.
    // < 0 means fatal error
    // = 0 means good, normal token
    // > 0 means unusual token
    const static int TOKEN_END_OF_FILE    = -1;
    const static int TOKEN_INVALID_CONFIGURATION = -2;
    const static int TOKEN_GOOD           = 0;
    const static int TOKEN_NO_MATCH_FOUND = 1;

    // Character tokenizing specifics.
    const static int CHAR_TOKEN      = 0;
    const static int CHAR_DELIM      = 1;
    const static int CHAR_WHITESPACE = 2;

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
    std::function< int(char) > getCharType;

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

    bool updateBuffer( size_t start = 0 );
    int getNextTokenPriv( LexicToken& tok );

    void setCustomWhitespacing(){
        // If using custom whitespaces, function must check for it.
        // Whitespace IS a delimiter.
        if( lexics.useCustomWhitespaces && !lexics.ignorables.empty() ){
            getCharType = [ & ] ( char c ) { 
                if(lexics.ignorables.find( c ) != std::string::npos)
                    return CHAR_WHITESPACE; 
                if(lexics.nonRegexDelimiters.find( c ) != std::string::npos)
                    return CHAR_DELIM;
                return CHAR_TOKEN;
            };
        }
        // If not, use standard whitespaces.
        else{
            getCharType = [ & ] ( char c ) { 
                if( std::iswspace( c ) )
                    return CHAR_WHITESPACE; 
                if(lexics.nonRegexDelimiters.find( c ) != std::string::npos)
                    return CHAR_DELIM;
                return CHAR_TOKEN; 
            };
        }
    }

public:
    // Move-semantic and Copy-semantic constructors.
    LexerImpl_SimpleDelim( const RegLexData& lexicData, std::istream& strm, bool useBQ=false)
        : useBlockingQueue( useBQ ), lexics( lexicData ), rdr( strm ), 
          bQueue( useBQ ? new gtools::BlockingQueue< LexicToken >() : nullptr )
    { setCustomWhitespacing(); }

    LexerImpl_SimpleDelim( RegLexData&& lexicData, std::istream& stream, bool useBQ = false )
        : useBlockingQueue( useBQ ), lexics( std::move(lexicData) ), rdr( stream ), 
          bQueue( useBQ ? new gtools::BlockingQueue< LexicToken >() : nullptr )
    { setCustomWhitespacing(); }

    void start();
    bool getNextToken( LexicToken& tok );
};

/*! A little helper for throwing errors.
 */ 
void LexerImpl_SimpleDelim::throwError( std::string message ){
    throw std::runtime_error( "[" + std::to_string( stats.lineCount ) +":"+
                             std::to_string( stats.posInLine )+"]: "+message );
}

/*! Updates the read buffer, and sets stream end flag, if ended.
 *  @param start - the offset from buffer's beginning, to which to write data.
 *  @return true, if read some data, false if no data was read.
 */ 
bool LexerImpl_SimpleDelim::updateBuffer( size_t start = 0 ){
    // If buffer is already exhausted, read stuff.
    if( bufferPointer >= bufferEnd ){
        if( start >= sizeof(buffer) ) // Fix start position if wrong.
            start = 0;

        rdr.read( buffer + start, sizeof(buffer) - start );
        size_t count = rdr.gcount();

        std::cout <<"[LexerImpl_SimpleDelim::updateBuffer()]: Updating buffer. ("<< count <<" chars).\n";

        // No chars were read - stream has ended. No more data can be got.
        if( count == 0 ){
            // Mark stream end.
            endOfStream = true;

            std::cout <<" Stream has ENDED ! \n\n";
            return false;
        }

        // Set buffer end to the point where the last read character is.
        bufferEnd = buffer + start + rdr.gcount();
        bufferPointer = buffer + start;

        std::cout <<"\n";
    }
    // Buffer is not exhausted - data still avairabru.
    return true;
}

/*! Inline f-on updating stream stats based on character got.
 */ 
inline void LexerImpl_SimpleDelim::updateLineStats( char c ){
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
int LexerImpl_SimpleDelim::getNextTokenPriv( LexicToken& tok ){
    // Change remaining buffer, and if empty, update it.
    if( !updateBuffer() ){
        // Update buffer returned false - the file has ended. Return END_OF_FILE.
        return TOKEN_END_OF_FILE;
    }

    // Iterate the buffer, searching for the delimiters, and at the same time 
    // update the statistics (line count, etc).
    std::cout <<"[LexerImpl_SimpleDelim::getNextToken()]: "<<
                "Buffpos: "<< (int)(bufferPointer - buffer) <<
                ", Bufflen: "<< (int)(bufferEnd - buffer) <<
                ", Streampos: "<< rdr.tellg() <<"\n";

    // Delimiter is defined as a char array. Tokens are separated by delimiters 
    const char* tokenStart = bufferPointer;
    
    // Loop the buffer, fetching new characters if needed.
    while( true ) {
        for( ; bufferPointer < bufferEnd; bufferPointer++ ){
            updateLineStats( *bufferPointer );

            // Get type of current character.
            int charType = getCharType( *bufferPointer );

            // Check if delimiter. Ignorable(Whitespace) is ALWAYS a delimiter.
            if( charType == CHAR_WHITESPACE || charType == CHAR_DELIM )
            {
                std::cout <<" Delimiter \'"<< *bufferPointer <<
                            "\' found at pos "<< bufferPointer-tokenStart <<"\n";
                
                // Token is not empty - perform regex-search on the current token.
                if( tokenStart < bufferPointer ){
                    std::cout <<" Searching for rule regex match... \n";

                    for( auto&& rule : lexics.rules ){
                        // If current token matches regex, fill up the LexicToken, and true.
                        if( std::regex_match( tokenStart, (const char*)(bufferPointer-1), 
                            rule.regex ) )
                        {
                            tok.id = rule.getID();
                            tok.data = std::string(tokenStart, (const char*)(bufferPointer-1));

                            // Woohoo! Found a valid token!
                            std::cout<< " MATCH FOUND! ID: "<< tok.id <<"\n\n"; 
                            return TOKEN_GOOD;
                        }
                    }

                    // No match found - invalid token.
                    tok.data = std::string( tokenStart, (const char*)(bufferPointer-1) );
                    tok.id = LexicToken::INVALID_TOKEN;

                    std::cout<<" No match was found! Token invalid!\n\n";
                    return TOKEN_NO_MATCH_FOUND;
                } 

                // If current deliminator is WS, the token start is after the current char.
                if( charType == CHAR_WHITESPACE ){
                    tokenStart = bufferPointer + 1;
                }
            }
        }

        // Check if token is too long (more than 75% of buffer's size.)
        if( bufferEnd - tokenStart >= BUFFER_SIZE*0.75

        // Buffer is exhausted, and token is still incomplete.
        // Try to fetch new characters from stream. Update current token start accordingly.
        if( updateBuffer() ){
            
        }
        // If stream has ended, finish token by explicitly inserting a whitespace.
        else{
             
        }
    } 

    return TOKEN_INVALID_CONFIGURATION;
}

/*! Runner function which fills the blocking queue with tokens.
 *  - Runs until the end of stream, or until error occurs.
 *  - CALLING RULES:
 *    - Can be called only when multithreading (using the Blocking Queue).
 *    - Also, only one instance of this function may execute at time.
 */ 
void LexerImpl_SimpleDelim::start(){
    // Check if call conditions are met (not running already, and using threads).
    if( running || !useBlockingQueue )
        return;
    running = true;

    while( 1 ){
        // Allocate a new token.
        LexicToken tok;
        int tt = getNextTokenPriv( tok );

        // If good, put to queue.
        if(tt >= 0)
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
bool LexerImpl_SimpleDelim::getNextToken( LexicToken& tok ){
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
 * TODO: Impl depending on delimiter type used - simple or regexed.
 */ 
Lexer::Lexer( const RegLexData& lexicData, std::istream& stream, bool useBQ )
    : impl( new LexerImpl_SimpleDelim( lexicData, stream, useBQ ) )
{}

Lexer::Lexer( RegLexData&& lexicData, std::istream& stream, bool useBQ )
    : impl( new LexerImpl_SimpleDelim( lexicData, stream, useBQ ) )
{}

void Lexer::start(){
    impl->start();
}

bool Lexer::getNextToken( LexicToken& tok ){
    return impl->getNextToken( tok );
}

}

