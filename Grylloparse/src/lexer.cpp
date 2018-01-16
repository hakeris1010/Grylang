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
    bool useLineStats = false;

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
    std::string buffer = std::string( BUFFER_SIZE, '\0' );
    const char* bufferPointer = &buffer[0];
    const char* bufferEnd = &buffer[0];

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
            if( lexics.regexed )
                getNextTokenPriv = getNextTokenPriv_Regexed ;
            else
                getNextTokenPriv = getNextTokenPriv_SimpleDelim;
        }

        // If using custom whitespaces, function must check for it.
        // Whitespace IS a delimiter.
        /*if( lexics.useCustomWhitespaces && lexics.regexWhitespaces.ready() ){
            getCharType = [ & ] ( char c ) { 
                if(lexics.whitespaces.find( c ) != std::string::npos)
                    return CHAR_WHITESPACE; 
                //if(lexics.nonRegexDelimiters.find( c ) != std::string::npos)
                //    return CHAR_DELIM;
                return CHAR_TOKEN;
            };
        }
        else if( lexics.useRegexWhitespaces ){
            getCharType = [ & ] ( char c ) { 
                if( std::regex_match( (const char*)&c, (const char*)&c, 
                                      lexics.regexWhitespaces.regex ) )
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
        }*/
    }

    static int getNextTokenPriv_SimpleDelim( LexerImpl& lex, LexicToken& tok );
    static int getNextTokenPriv_Regexed( LexerImpl& lex, LexicToken& tok );

public:
    /*! Move-semantic and Copy-semantic constructors.
     *  @param lexicData - RegLex-type structure, containing data necessary for lexing.
     *                     Based on Regex. Most likely created from GBNF.
     *
     *  @param strm - C++ input stream from where to take data.
     *  @param useBQ - Enable multithreaded mode (Run start() on separate worker thread,
     *                 call getNextToken() from main thread).  
     *
     *  @param getNxTk - Supplied next token getter function.
     *                   Must comply to the Lexer API.
     */ 
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
        if( start >= buffer.size() ) // Fix start position if wrong.
            start = 0;

        rdr.read( &buffer[0] + start, buffer.size() - start );
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
        bufferEnd = &buffer[0] + start + count;
        bufferPointer = &buffer[0] + start;

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
    std::cout <<"[LexerImpl::getNextTokenPriv_SimpleDelim()]: This is Shit!\n";
    return TOKEN_INVALID_CONFIGURATION;
}
    
    // Firstly, skip all leading Whitespaces.
/*    while(true){
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
            lex.buffer[0] = ( lex.lexics.useCustomWhitespaces ? lex.lexics.whitespaces[0] : ' ' );
            lex.bufferPointer = lex.buffer;
            lex.bufferEnd = lex.buffer+1;
        }
    } 

    return TOKEN_INVALID_CONFIGURATION;
}
*/


/*! Main backend function for the universal, RegEx'd tokenizing.
 *  Does all the true job of getting the tokens.
 *  - Uses the RegLex model v0.2.
 *  - Requires that the provided RegLex data include these properties:
 *   1. Full Language Regex.
 *    - It's a regex which is used to match all valid tokens of the language.
 *    - Tokens are matched as ORed Capture Groups. The Group's Index is mapped to Token's ID.
 *    - Fallback Error group is used, to identify errors.
 *   2. Group Index - Token ID map.
 *   3. Error group index.
 *   4. Whitespace group index. 
 *
 *  - CAUTION: No checking is done on the RegLex data. 
 *             The user must provide the valid above sections.
 */ 
int LexerImpl::getNextTokenPriv_Regexed( LexerImpl& lex, LexicToken& tok ){
    std::cout <<"[LexerImpl::getNextTokenPriv_Regexed()]: Using the Full Language Regex.\n";

    // Updating buffer.
    lex.updateBuffer();

    std::cout <<" Buffpos: "<< (int)(lex.bufferPointer - &(lex.buffer[0])) <<
                ", Bufflen: "<< (int)(lex.bufferEnd - &(lex.buffer[0])) <<
                ", Streampos: "<< lex.rdr.tellg() <<"\n";

    // Mark token as Invalid in the beginning.
    tok.id = LexicToken::INVALID_TOKEN; 

    // If buffer was extended (token is longer that one fetch-length).
    bool bufferWasExtended = false;

    // Run while there are buffers to be extrated from stream.
    while( true ) {
        // Run this while in buffer. Extract tokens until valid one.
        // Match regex. Use group's index to find out Token's ID.
        std::cmatch m;
        bool reBufferNeeded = false;

        while( (lex.bufferPointer < lex.bufferEnd) && !reBufferNeeded ){
            //m.clear();
            if( !std::regex_search( lex.bufferPointer, lex.bufferEnd, m, 
                                    lex.lexics.fullLanguageRegex.regex) ) 
                lex.throwError( "REGEX Can't be matched. Maybe language regex is wrong." );

            // Find the group which was matched. Then use the index to set the ID.
            // Start from 1st submatch, because 0th is the whole regex match.
            for (size_t i = 1; i < m.size(); i++){
                if( m[i].length() > 0 ){
                    std::cout << " Regex Group #"<< i - 1 <<" was matched.\n";

                    // Check if it's a whitespace. If so, match next token.
                    if( i - 1 == lex.lexics.spaceRuleIndex ){
                        lex.bufferPointer += m.position() + m.length();
                        break;
                    }

                    // Check if the token has reached buffer end, but not file end.
                    // If so, reBuffering is needed. Break the loop and fetch new data.
                    const char* tokEnd = lex.bufferPointer + m.position() + m.length();
                    if( (tokEnd >= lex.bufferEnd) && !lex.endOfStream ){
                        reBufferNeeded = true;
                        break;
                    }

                    // At this point, Token ends before the end of the buffer/stream.
                    // Check if error rule was matched. Throw an XcEpTiOn.
                    else if( i - 1 == lex.lexics.errorRuleIndex ){
                        // Find out line position.

                        std::cout<<" ERROR! Token \""<< m[i] << "\" matched the Error Group!\n";
                        lex.throwError( "Invalid token." );
                    } 

                    // It's good at this point. Fill the data.
                    tok.id = lex.lexics.tokenTypeIDs[ i - 1 ];

                    // Check if buffer has been extended. 
                    // If so, the buffer will need to be shrinked later. 
                    // So, std::move the data from the buffer to token's data,
                    // and then reset the buffer.
                    if( bufferWasExtended ){
                        std::cout<<" Buffer was extended. Resetting sizes, std::move token.\n";

                        // Copy remaining buffer.
                        size_t tokp = (lex.bufferPointer - lex.buffer.c_str()) + m.position();
                        size_t remLen = lex.bufferEnd - tokEnd;

                        std::string remBuff( BUFFER_SIZE, '\0' );
                        remBuff.assign( tokEnd, remLen );

                        if( tokp > 0 ){
                            // Construct from buffer (Copy n bytes).
                            tok.data.assign( std::move( lex.buffer ), tokp, m.length() );
                        }
                        else{ // Token starts at position ZERO.
                            tok.data.assign( std::move( lex.buffer ) );
                            tok.data.resize( m.length() );
                        }

                        // Reset the buffer to start size.
                        lex.buffer.assign( std::move( remBuff ) );

                        lex.bufferPointer = &(lex.buffer[0]);
                        lex.bufferEnd = lex.bufferPointer + remLen;
                    }
                    else{
                        tok.data = std::move( m[i].str() ); 

                        // Buffer hasn't ended - we're sure that token ends here. 
                        // Update buffer pointers.
                        lex.bufferPointer = tokEnd; 
                    }

                    return LexerImpl::TOKEN_GOOD;
                }
            }
        }

        // Offset in the buffer to where to fetch the data to.
        size_t fetchOffset = 0;

        // At this point either data buffer is exhausted, or token is too long,
        // so ReBuffering is needed. Extend buffer by another BUFFER_SIZE bytes.
        if( reBufferNeeded ){
            size_t curTokStart = (lex.bufferPointer - lex.buffer.c_str()) + m.position();
            lex.buffer.resize( lex.buffer.size() + LexerImpl::BUFFER_SIZE, '\0' );

            // Move memory to start of the buffer, if the token starts later.
            if( curTokStart > 0 )
                std::memmove( &(lex.buffer[0]), &(lex.buffer[0]) + curTokStart, m.length() );

            // Fetch data to this place.
            fetchOffset = m.length();

            // Mark this flag, for reset after valid token is found.
            bufferWasExtended = true;

            std::cout << " Token was too long. ReBuffered. New BufLen: "<< lex.buffer.size()
                      << ", token length: "<< m.length() <<"\n";  
        }

        // Fetch the new data from stream. All pointers will automatically be assigned.
        if( !lex.updateBuffer( fetchOffset ) ) //&& tok.id == LexicToken::INVALID_TOKEN )
            return TOKEN_END_OF_FILE;

        // If buffer was extended (token is too long), 
        // the data which needs to be checked starts at the beginning.
        if( reBufferNeeded ){
            lex.bufferPointer = &(lex.buffer[0]);
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

