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
class LexerImpl : public BaseLexer
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
    const static int TOKEN_PARTIAL        = 2;

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
    
    // State variables
    volatile bool running = false; 
    volatile bool endOfStream = false;

    // Stream state specifics.
    StreamStats stats;    

    // Token's store buffer.
    char buffer[ BUFFER_SIZE ];
    char* bufferPointer = buffer;
    const char* bufferEnd = buffer;
     

    // Backend functions.
    void throwError( std::string message );
    inline void updateLineStats( char c );

    bool updateBuffer( size_t start = 0 );

    // Tokenizing parameters
    std::function< int(char) > getCharType; 

    // TODO: Use dynamically assigned Tokenizers,
    // or even use a separate class for tokenizing.
    std::function< int(LexerImpl&, LexicToken&) > getNextTokenPriv; 

    void setFunctions(){
        // Set tokenizer.
        if( !getNextTokenPriv ){
            if( lexics.useRegexDelimiters )
                getNextTokenPriv = getNextTokenPriv_Regexed ;
            else
                getNextTokenPriv = getNextTokenPriv_SimpleDelim;
        }

        // If using custom whitespaces, function must check for it.
        // Whitespace IS a delimiter.
        if( lexics.useCustomWhitespaces && !lexics.ignorables.empty() ){
            getCharType = [ & ] ( char c ) { 
                if(lexics.ignorables.find( c ) != std::string::npos)
                    return CHAR_WHITESPACE; 
                //if(lexics.nonRegexDelimiters.find( c ) != std::string::npos)
                //    return CHAR_DELIM;
                return CHAR_TOKEN;
            };
        }
        // If not, use standard whitespaces.
        else{
            getCharType = [ & ] ( char c ) { 
                if( std::iswspace( c ) )
                    return CHAR_WHITESPACE; 
                //if(lexics.nonRegexDelimiters.find( c ) != std::string::npos)
                //    return CHAR_DELIM;
                return CHAR_TOKEN; 
            };
        }
    }

    static int getNextTokenPriv_SimpleDelim( LexerImpl& lex, LexicToken& tok );
    static int getNextTokenPriv_Regexed( LexerImpl& lex, LexicToken& tok );

public:
    // Move-semantic and Copy-semantic constructors.
    LexerImpl( const RegLexData& lexicData, std::istream& strm, bool useBQ=false, 
               const std::function< int(LexerImpl&, LexicToken&) >& getNxTk = 
                     std::function< int(LexerImpl&, LexicToken&) >() )
        : useBlockingQueue( useBQ ), lexics( lexicData ), rdr( strm ), 
          bQueue( useBQ ? new gtools::BlockingQueue< LexicToken >() : nullptr ),
          getNextTokenPriv( getNxTk )
    { setFunctions(); }

    LexerImpl( RegLexData&& lexicData, std::istream& stream, bool useBQ = false,
               std::function< int(LexerImpl&, LexicToken&) >&& getNxTk = 
                    std::function< int(LexerImpl&, LexicToken&) >() )
        : useBlockingQueue( useBQ ), lexics( std::move(lexicData) ), rdr( stream ), 
          bQueue( useBQ ? new gtools::BlockingQueue< LexicToken >() : nullptr ),
          getNextTokenPriv( std::move( getNxTk ) )
    { setFunctions(); }

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
 *  @param start - the offset from buffer's beginning, to which to write data.
 *  @return true, if read some data, false if no data was read.
 */ 
bool LexerImpl::updateBuffer( size_t start ){
    // If buffer is already exhausted, read stuff.
    if( bufferPointer >= bufferEnd ){
        if( start >= sizeof(buffer) ) // Fix start position if wrong.
            start = 0;

        rdr.read( buffer + start, sizeof(buffer) - start );
        size_t count = rdr.gcount();

        std::cout <<"[LexerImpl::updateBuffer()]: Updating buffer. ("<< count <<" chars).\n";

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
inline void LexerImpl::updateLineStats( char c ){
    if( c == '\n' ){
        stats.lineCount++;
        stats.posInLine = 0;
    }
    else
        stats.posInLine++;
}

/*! Main backend function.
 *  FIXME: The whole delimited token design is flawed.
 *         It works only when all tokens are separated by, let's say, whitespaces.
 *         In practical use, it's not feasible. 
 *
 *  Does all the true job of getting the tokens.
 *  - Iterates the buffer, searching for the delimiters, and at the same time 
 *    updates the statistics (line count, etc).
 */ 
int LexerImpl::getNextTokenPriv_SimpleDelim( LexerImpl& lex, LexicToken& tok ){
    std::cout <<"[LexerImpl::getNextToken()]: Skipping WS...\n";
    
    // Firstly, skip all leading Whitespaces.
    while(true){
        for( ; lex.bufferPointer < lex.bufferEnd; lex.bufferPointer++ ){
            lex.updateLineStats( *lex.bufferPointer );

            if( lex.getCharType( *lex.bufferPointer ) != CHAR_WHITESPACE )
                goto _wsLoopEnd;
        }

        // Buffer exhausted. Update lex.buffer - fetch new data.
        if( !lex.updateBuffer() ){
            // Update lex.buffer returned false - the file has ended. 
            return TOKEN_END_OF_FILE;
        }
    } 
    _wsLoopEnd:

    // Increment lex.buffer pointer, to proceed to other character after the 
    ++lex.bufferPointer;
     
    std::cout <<"After WS skip: Buffpos: "<< (int)(lex.bufferPointer - lex.buffer) <<
                ", Bufflen: "<< (int)(lex.bufferEnd - lex.buffer) <<
                ", Streampos: "<< lex.rdr.tellg() <<"\n";
    
    // At this point, the lex.buffer pointer points to a non-whitespace character.
    // Mark current position as token start, and go on to the next character.
    const char* tokenStart = lex.bufferPointer;
    ++lex.bufferPointer;

    while( true ) {
        for( ; lex.bufferPointer < lex.bufferEnd; lex.bufferPointer++ ){
            lex.updateLineStats( *lex.bufferPointer );

            // Get type of current character.
            int charType = lex.getCharType( *lex.bufferPointer );

            // Check if delimiter. Ignorable(Whitespace) is ALWAYS a delimiter.
            // We know that Token is not empty - perform regex-search on the current token.
            if( charType != CHAR_TOKEN )
            {
                std::cout <<" Delimiter \'"<< *lex.bufferPointer 
                          <<"\' found at pos "<< lex.bufferPointer-tokenStart <<"\n"
                          <<" Searching for rule regex match... \n";

                for( auto&& rule : lex.lexics.rules ){
                    // If current token matches regex, fill up the LexicToken, and true.
                    if( std::regex_match( tokenStart, (const char*)(lex.bufferPointer-1), 
                        rule.regex ) )
                    {
                        tok.id = rule.getID();
                        tok.data = std::string(tokenStart, (const char*)(lex.bufferPointer-1));

                        // Woohoo! Found a valid token!
                        std::cout<< " MATCH FOUND! ID: "<< tok.id <<"\n\n"; 
                        return TOKEN_GOOD;
                    }
                }

                // No match found - invalid token.
                tok.data = std::string( tokenStart, (const char*)(lex.bufferPointer-1) );
                tok.id = LexicToken::INVALID_TOKEN;

                std::cout<<" No match was found! Token invalid!\n\n";
                return TOKEN_NO_MATCH_FOUND;
            }
        }

        size_t toksize = (size_t)( lex.bufferEnd - tokenStart );

        // Check if token is too long (more than 75% of lex.buffer's size.)
        if( toksize >= (int)((double)BUFFER_SIZE * 0.75) )
            break;
        // If not, reallocate memory and update lex.buffer.
        else{
            std::memmove( lex.buffer, tokenStart, toksize );
            tokenStart = lex.buffer;
        }

        // Buffer is exhausted, and token is still incomplete.
        // If stream has ended, finish token by explicitly inserting a whitespace.
        if( !lex.updateBuffer( toksize ) ){
            lex.buffer[0] = ( lex.lexics.useCustomWhitespaces ? lex.lexics.ignorables[0] : ' ' );
            lex.bufferPointer = lex.buffer;
            lex.bufferEnd = lex.buffer+1;
        }
    } 

    return TOKEN_INVALID_CONFIGURATION;
}

/*! Main backend function for the universal, RegEx'd tokenizing.
 *  Does all the true job of getting the tokens.
 *  - Iterates the lex.buffer, getting characters one by one, and checking if current string
 *    matches at least one regex.
 *  - Main disadvantage - the time is exponential depending on token's lenght,
 *    in other words, O(n^m), where 
 *    - m is lenght of the token,
 *    - n is the count of rules.
 */ 
int LexerImpl::getNextTokenPriv_Regexed( LexerImpl& lex, LexicToken& tok ){
    std::cout <<"[LexerImpl::getNextTokenPriv_Regexed()]: Skipping WS...\n";
    
    // Firstly, skip all leading Whitespaces.
    while(true){
        for( ; lex.bufferPointer < lex.bufferEnd; lex.bufferPointer++ ){
            lex.updateLineStats( *lex.bufferPointer );

            if( lex.getCharType( *lex.bufferPointer ) != CHAR_WHITESPACE )
                goto _wsLoopEnd;
        }

        // Buffer exhausted. Update lex.buffer - fetch new data.
        if( !lex.updateBuffer() ){
            // Update lex.buffer returned false - the file has ended. 
            return TOKEN_END_OF_FILE;
        }
    } 
    _wsLoopEnd:

    // Increment lex.buffer pointer, to proceed to other character after the 
    ++lex.bufferPointer;
     
    std::cout <<"After WS skip: Buffpos: "<< (int)(lex.bufferPointer - lex.buffer) <<
                ", Bufflen: "<< (int)(lex.bufferEnd - lex.buffer) <<
                ", Streampos: "<< lex.rdr.tellg() <<"\n";
    
    // At this point, the lex.buffer pointer points to a non-whitespace character.
    // Mark current position as token start, and go on to the next character.
    const char* tokenStart = lex.bufferPointer;
    ++lex.bufferPointer;

    while( true ) {
        for( ; lex.bufferPointer < lex.bufferEnd; lex.bufferPointer++ ){
            // Character got. Now perform checks.
            lex.updateLineStats( *lex.bufferPointer );

            // We don't need to check the type of current character.
            // We just need to check if current token matches at least one regex.
            // If there were some errors, but token is still "found" inside, we will 
            // still use that token.
            std::cmatch results;
            int firstPartialRule = -1;
            
            for( auto&& rule : lex.lexics.rules ){
                if( std::regex_search( tokenStart, (const char*)(lex.bufferPointer), 
                                       results, rule.regex ) )
                {
                    // Examine position. If position is not start, check other rules,
                    // But remember the ID of this one too.
                    if( results.position() != 0 && firstPartialRule < 0 ){
                        firstPartialRule = rule.getID();
                        continue;
                    }
                    
                    // If position is start, it means token is fully good.
                    tok.id = rule.getID();
                    tok.data = std::string(tokenStart, (const char*)(lex.bufferPointer));

                    // Woohoo! Found a valid token!
                    std::cout <<" MATCH FOUND! At: "<< (size_t)(lex.bufferPointer - tokenStart) 
                              <<", ID: "<< tok.id <<"\n\n"; 
                    return TOKEN_GOOD;
                }
            }

            // Partial match was found. It means there were illegal characters on a token.
            if( firstPartialRule >= 0 ){
                tok.id = firstPartialRule;
                tok.data = std::string(tokenStart, (const char*)(lex.bufferPointer));

                // Woohoo! Found a valid token!
                std::cout <<" MATCH FOUND! At: "<< (size_t)(lex.bufferPointer - tokenStart) 
                          <<", ID: "<< tok.id <<"\n\n"; 
                return TOKEN_PARTIAL; 
            }
        }

        // Check if token is too long (more than 75% of lex.buffer's size.)
        size_t toksize = (size_t)( lex.bufferEnd - tokenStart );

        if( toksize >= (int)((double)BUFFER_SIZE * 0.75) )
            break;
        // If not, reallocate memory and update lex.buffer.
        else{
            std::memmove( lex.buffer, tokenStart, toksize );
            tokenStart = lex.buffer;
        }
    } 

    // Mark token as Invalid.
    tok.id = LexicToken::INVALID_TOKEN;

    return TOKEN_INVALID_CONFIGURATION;
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
        int tt = getNextTokenPriv( *this, tok );

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
bool LexerImpl::getNextToken( LexicToken& tok ){
    // If using multithreading, wait until token gets put into the queue.
    // If stream has ended, but queue has tokens, extract them.
    if( useBlockingQueue ){
        if(!endOfStream || !bQueue->isEmpty())
            tok = bQueue->pop();
        return !endOfStream || !bQueue->isEmpty();
    }
    
    // If no multithreading, just directly call the private token getter.
    int ret = getNextTokenPriv( *this, tok );

    // return true (have more tokens) if normal token or non-fatal error.
    return ret >= 0 && !endOfStream;
}

/*=============================================================
 * Public Lexer methods.
 * TODO: Impl depending on delimiter type used - simple or regexed.
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

