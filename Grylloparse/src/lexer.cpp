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
    const static size_t BUFFER_SIZE = 5;

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
    // Mode properties.
    const bool useBlockingQueue;
    
    const bool useLineStats = false;
    const bool useDedicatedLoopyTokenizer = true;

    const int verbosity = 3;

    // Lexic Data (RegLex-format).
    const RegLexData lexics;
    
    // Stream from which to take data.
    std::istream& rdr;

    // We'll use this only if useBlockingQueue.
    std::unique_ptr< gtools::BlockingQueue< LexicToken > > bQueue;
    
    // State variables
    // Running - used in multithreading. Indicates whether the Extractor Thread 
    //           is still running.
    volatile bool running = false; 

    // Indicates whether Stream has Ended (EOF).
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

    // Dynamically assigned tokenizer and iterative loop tokenizer implementations
    std::function< int(LexerImpl&, LexicToken&) > getNextTokenPriv; 
    std::function< void( LexerImpl& ) > runnerPriv; 

    void setFunctions(){
        // Set single-token tokenizer.
        if( !getNextTokenPriv ){
            if( lexics.regexed )
                getNextTokenPriv = getNextTokenPriv_Regexed ;
            else
                getNextTokenPriv = getNextTokenPriv_SimpleDelim;
        }

        // Set iterative runner tokenizer.
        if( useDedicatedLoopyTokenizer )
            runnerPriv = runner_dedicatedIteration;
        else
            runnerPriv = runner_usingTokenGetter;
    }

    static int getNextTokenPriv_SimpleDelim( LexerImpl& lex, LexicToken& tok );
    static int getNextTokenPriv_Regexed( LexerImpl& lex, LexicToken& tok );

    static void runner_dedicatedIteration( LexerImpl& lex );
    static void runner_usingTokenGetter( LexerImpl& lex );

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
    if( endOfStream )
        return false;

    // If buffer is already exhausted, read stuff. 
    // "Start" position overrides the buffer pointers.
    if( (bufferPointer >= bufferEnd) || start ){
        if( start >= buffer.size() ) // Fix start position if wrong.
            start = 0;

        rdr.read( &buffer[0] + start, buffer.size() - start );
        size_t count = rdr.gcount();

        if( verbosity > 0 )
            std::cout <<"[LexerImpl::updateBuffer()]: Updating buffer. ("<< count <<" chars).\n";

        // Check for EOF. If yes, mark end of stream.
        if( rdr.eof() ){
            endOfStream = true;
        }

        // No chars were read - stream has ended. No more data can be got.
        if( count == 0 ){
            if( verbosity > 0 )
                std::cout <<" Stream has ENDED ! \n\n";
            return false;
        }

        // Set buffer end to the point where the last read character is.
        bufferEnd = &buffer[0] + start + count;
        bufferPointer = &buffer[0] + start;

        if( verbosity > 0 )
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

/*! backend function.
 *  FIXME: The whole delimited token design is flawed.
 *         It works only when all tokens are separated by, let's say, whitespaces.
 *         In practical use, it's not feasible. 
 *
 *  - Iterates the buffer, searching for the delimiters, and at the same time 
 *    updates the statistics (line count, etc).
 */ 
int LexerImpl::getNextTokenPriv_SimpleDelim( LexerImpl& lex, LexicToken& tok ){
    if( lex.verbosity > 0 )
        std::cout <<"[LexerImpl::getNextTokenPriv_SimpleDelim()]: This is Shit!\n";
    return TOKEN_INVALID_CONFIGURATION;
}

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
    if( lex.verbosity > 0 )
        std::cout <<"[LexerImpl::getNextTokenPriv_Regexed()]: Using the Full Language Regex.\n";

    // Updating buffer, and check if we can read something.
    if( !lex.updateBuffer() && (lex.bufferPointer >= lex.bufferEnd) ){
        if( lex.verbosity > 0 )
            std::cout <<" No data to read!\n\n";
        return TOKEN_END_OF_FILE;
    }

    if( lex.verbosity > 1 ){
        std::cout <<" Buffpos: "<< (int)(lex.bufferPointer - &(lex.buffer[0])) <<
                    ", Bufflen: "<< (int)(lex.bufferEnd - &(lex.buffer[0])) <<
                    ", Streampos: "<< lex.rdr.tellg() <<"\n";
    }

    // Mark token as Invalid in the beginning.
    //tok.id = LexicToken::INVALID_TOKEN; 

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
                                    lex.lexics.fullLanguageRegex.regex) ){
                // If using a fallback group, and still can't match, then regex is wrong.
                if( lex.lexics.useFallbackErrorRule )
                    lex.throwError( "REGEX Can't be matched. Maybe language regex is wrong." );

                // Refetch buffer.
                lex.bufferPointer = lex.bufferEnd;
                break;
            }

            // Find the group which was matched. Then use the index to set the ID.
            // Start from 1st submatch, because 0th is the whole regex match.
            for (size_t i = 1; i < m.size(); i++){
                if( m[i].length() > 0 ){
                    if( lex.verbosity > 1 )
                        std::cout << " Regex Group #"<< i - 1 <<" was matched, at pos: " \
                                  << m.position() << " from ptr.\n" ;
                    if( lex.verbosity > 2 && lex.buffer.size() < 40 )
                        std::cout << " Buffer from Pointer: \""<< lex.bufferPointer << "\"\n\n";

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

                    // At this point, Token ends before the end of the buffer,
                    // or the stream has actually ended, so this is the valid end too.
                    
                    // Check if error rule was matched. Throw an XcEpTiOn.
                    else if( lex.lexics.useFallbackErrorRule &&
                             i - 1 == lex.lexics.errorRuleIndex )
                    {
                        // TODO: Find out line position.

                        if( lex.verbosity > 0 )
                            std::cout<<" ERROR! Token \""<< m[i] << "\" matched the Error Group!\n";
                        lex.throwError( "Invalid token." );
                    } 

                    // It's good at this point. Fill the data.
                    tok.id = lex.lexics.tokenTypeIDs[ i - 1 ];

                    // Check if buffer has been extended (ReBuffered when token was longer 
                    // than half of the buffer).
                    // If so, the buffer will need to be shrinked later. 
                    // So, std::move the data from the buffer to token's data,
                    // and then reset the buffer.
                    if( bufferWasExtended ){
                        if( lex.verbosity > 2 ){
                            std::cout<<" Buffer was extended. std::move buffer to token data.\n";
                            if( lex.buffer.size() < 50) std::cout<<" Buffer: "<<lex.buffer<<"\n";
                        }

                        // Copy remaining buffer.
                        size_t tokp = (lex.bufferPointer - lex.buffer.c_str()) + m.position();
                        size_t remLen = lex.bufferEnd - tokEnd;

                        std::string remBuff( BUFFER_SIZE, '\0' );
                        std::memmove( &(remBuff[0]), tokEnd, remLen );

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

                        if( lex.verbosity > 2 ){
                            std::cout<<" BuffSize after std::moving: "<<lex.buffer.size()<<"\n";
                            if(lex.buffer.size() < 50) std::cout<<" Buffer: "<<lex.buffer<<"\n";
                        }
                    }
                    else{
                        tok.data = std::move( m[i].str() ); 

                        // Buffer hasn't ended - we're sure that token ends here. 
                        // Update buffer pointers.
                        lex.bufferPointer = tokEnd; 
                    }

                    if( lex.verbosity > 0 ){
                        std::cout<<" Token Matched! "<< "ID: "<< tok.id << ", data: " <<
                            ( tok.data.size() < 30 ? tok.data : 
                              "("+std::to_string( tok.data.size() )+")" ) << "\n\n";
                    }
                    return LexerImpl::TOKEN_GOOD;
                }
            }
        }

        // Offset in the buffer to where to fetch the data to.
        size_t fetchOffset = 0;

        /* At this point either data buffer is exhausted, or token is too long.
         * ( bufferPointer == bufferEnd ), so ReBuffering is needed. 2 options:
         * 1. Toke is SHORTER than BufferSize/2
         * 2. Token is LONGER than BufferSize/2. Extend buffer by BUFFER_SIZE/2 bytes.
         *    This way we will be able to get additional max. BUFFER_SIZE data from stream.
         *    When token is finished and we shrink buffer again, the remaining data will
         *    be shorter than BUFFER_SIZE, so it fits.
         */
        if( reBufferNeeded ){
            if( lex.verbosity > 2 ){
                std::cout<<" Token ends at buffer end. ReBuffering needed.\n";
                if( lex.buffer.size() < 50 ) std::cout<< " Buffer Before: "<<lex.buffer<<"\n";
            }

            size_t curTokStart = (lex.bufferPointer - &(lex.buffer[0])) + m.position();

            // If token is longer than half of BUFFER_SIZE, extend the buffer.
            if( (size_t)(m.length()) > (size_t)(lex.buffer.size() - LexerImpl::BUFFER_SIZE/2) ){
                if( lex.verbosity > 2 )
                    std::cout<< " Extending Buffer.\n";

                // Move memory to the resized buffer, if the token starts later.
                if( curTokStart > 0 ){
                    // Create new buffer
                    std::string tmp( lex.buffer.size() + (LexerImpl::BUFFER_SIZE / 2), '\0' );

                    // Move data to new buffer.
                    std::memmove( &(tmp[0]), &(lex.buffer[0]) + curTokStart, m.length() );

                    // Assign the new buffer to the lex.buffer (MOVE, no copies).
                    lex.buffer.assign( std::move( tmp ) );
                }
                else
                    lex.buffer.resize( lex.buffer.size() + (LexerImpl::BUFFER_SIZE / 2), '\0' );

                // Mark this flag, for buffer eset after valid token is matched.
                bufferWasExtended = true;
            }
            // If token is not longer than, no resizes needed.
            // Move memory to start of the buffer, if the token starts later.
            else if( curTokStart > 0 )
                std::memmove( &(lex.buffer[0]), &(lex.buffer[0]) + curTokStart, m.length() );
             
            // Fetch data to this place.
            fetchOffset = m.length();

            if( lex.verbosity > 2 ){
                std::cout << " After ReBuffering: New BufLen: "<< lex.buffer.size()
                          << ", token length: "<< m.length() <<"\n";  
                if( lex.buffer.size() < 50 ) std::cout<< " Buffer After: "<<lex.buffer<<"\n";
            }
        }

        // Fetch the new data from stream. All pointers will automatically be assigned.
        if( !lex.updateBuffer( fetchOffset ) ){
            if( !bufferWasExtended )
                return TOKEN_END_OF_FILE;

            // However if the buffer was extended, and no data were extracted,
            // We must ReSet the end of buffer to the point where current data ends.
            if( reBufferNeeded )
                lex.bufferEnd =  &(lex.buffer[0]) + fetchOffset;
        }

        // If buffer was extended (token was too long).
        // the data which needs to be checked starts at the beginning.
        if( reBufferNeeded || bufferWasExtended ){
            lex.bufferPointer = &(lex.buffer[0]);
        }
    } 

    return TOKEN_INVALID_CONFIGURATION;
}

/*! Runner function with dedicated iterative loop mechanism,
 *  to avoid overhead of calling 250-line single token getter
 *  every time.
 *  - For calling conditions, look on public interface method.
 */ 
void LexerImpl::runner_dedicatedIteration( LexerImpl& lex ){
    if( lex.verbosity > 0 )
        std::cout << "[LexerImpl::runner_dedicatedIteration()]: Starting the Harvesting!\n"; 

    // Perform an initial buffer update.
    if( !lex.updateBuffer() ){
        if( lex.verbosity > 0 )
            std::cout << " No more data to read!\n\n";
        return;
    }

    // Constants needed for rebuffer modes.
    const char REFETCH_NEEDED   = 1;
    const char TOKEN_AT_THE_END = 2;

    // Status variables.
    size_t matches = 0;
    size_t bufferUpdates = 0;
    bool bufferWasExtended = false;

    // The main loop - performs buffer updates and extends.
    while( ++bufferUpdates ){
        char reBufferNeeded = 0;
        const char* tokEnd = lex.bufferPointer;
        size_t fetchOffset = 0;

        // The Matching loop - performs regex matches in a buffer.
        // Run while current token iterator is valid (not end of buffer)
        // - Notice that at the end of loop we increment only the "Next Token"
        //   iterator, and assign current token iterator from the next token iterator.
        // - So only one regex match is being done at each loop.
        for( auto it = std::cregex_iterator( lex.bufferPointer, lex.bufferEnd, 
                                             lex.lexics.fullLanguageRegex.regex );
                  !reBufferNeeded && (it != std::cregex_iterator()); 
                  ++it, ++matches ) 
        {
            // Current match.
            auto&& m = *it;

            if( lex.verbosity > 1 ){
                std::cout << "\nMATCH FOUND: "; 
                if( m.length() < 50 ) 
                    std::cout <<"\""<< m.str() <<"\"\n";
                else 
                    std::cout <<"("<< m.length() <<" characters)\n"; 
                std::cout <<" At position " << m.position() <<", Length: "<< m.length();
            }

            // Check which group was matched. By that set token's ID.
            for( size_t i = 1; i < m.size(); i++ ){
                if( m[i].length() ) {
                    if( lex.verbosity > 1 )
                        std::cout << ", Capture group index: " << i - 1 << "\n";

                    // Check if it's a whitespace. If so, match next token.
                    if( i - 1 == lex.lexics.spaceRuleIndex ){
                        if( lex.verbosity > 2 )
                            std::cout << " Whitespace group was matched! Skipping...\n";
                        break;
                    }

                    // Check if the token has reached buffer end, but not file end.
                    // If so, reBuffering is needed. Break the loop and fetch new data.
                    tokEnd = lex.bufferPointer + m.position() + m.length();
                    if( (tokEnd >= lex.bufferEnd) && !lex.endOfStream ){
                        if( lex.verbosity > 2 )
                            std::cout << " Token match reached the end of the buffer." \
                                      << " ReBuffering is needed.\n";
                        // Set bufferPointer to token's start, for easier data moving.
                        reBufferNeeded = TOKEN_AT_THE_END;
                        lex.bufferPointer += m.position();
                        break;
                    }

                    // At this point, Token ends before the end of the buffer,
                    // or the stream has actually ended, so this is the valid end too.

                    // Check if error rule was matched. Throw an XcEpTiOn.
                    if( lex.lexics.useFallbackErrorRule &&
                        i - 1 == lex.lexics.errorRuleIndex )
                    {
                        // TODO: Find out line position.

                        if( lex.verbosity > 0 )
                            std::cout << " ERROR! Token \""<< m[i] \
                                      << "\" matched the Error Group!\n";
                        lex.throwError( "Invalid token." );
                    } 

                    // Token is good at this point. Fill the data.
                    LexicToken tok;

                    tok.id = lex.lexics.tokenTypeIDs[ i - 1 ];
                    
                    // Reset the buffer when job is done, if was extended.
                    // If buffer was extended, the token starts AT BEGINNING OF THE BUFFER.
                    if( bufferWasExtended ){
                        if( lex.verbosity > 2 )
                            std::cout<< " Buffer was extended. Shrinking and std::moving.";

                        tok.data.assign( std::move( lex.buffer ) );

                        // Move remaining data to new buffer.
                        lex.buffer.assign( LexerImpl::BUFFER_SIZE, '\0' );
                        std::memmove( &(lex.buffer[0]), tokEnd, (lex.bufferEnd - tokEnd) );

                        // Now we can safely resize the token buffer.
                        tok.data.resize( m.length() );

                        reBufferNeeded = REFETCH_NEEDED;
                        fetchOffset = (lex.bufferEnd - tokEnd);

                        bufferWasExtended = false;

                        if( lex.verbosity > 2 )
                            std::cout<< " New length: "<< lex.buffer.size() <<"\n";
                    }
                    else{
                        // m.str() - Create this match's std::string (copy bytes from 
                        //           buffer to std::string,
                        // assign( std::move( ... ) ) - Move the data (assign pointer to buffer)
                        //           from the string created in m.str() to tok.data.
                        tok.data.assign( std::move( m.str() ) ); 
                    }

                    // Push token to queue.
                    lex.bQueue->push( tok );

                    break;
                }
            }
        }
         
        // Take care of buffer shifting/extending, and data fetching.
        // If ReBuffering is needed, then token start/end positions are already known.
        // Move token to the start of the buffer.
        if( reBufferNeeded == TOKEN_AT_THE_END ){
            if( lex.verbosity > 2 ){
                std::cout<<" ReBuffering.";
                if( lex.buffer.size() < 50 ) std::cout<< " Buffer Before: "<<lex.buffer;
                std::cout<<"\n";
            }

            // Token length is stored in fetchOffset.
            fetchOffset = tokEnd - lex.bufferPointer;

            // If token is longer than half of BUFFER_SIZE, extend the buffer.
            if( fetchOffset > (size_t)(lex.buffer.size() - LexerImpl::BUFFER_SIZE/2) ){
                if( lex.verbosity > 2 )
                    std::cout<< " Extending Buffer.\n";

                // Move memory to the resized buffer, if the token starts later.
                if( fetchOffset > 0 ){
                    // Create new buffer
                    std::string tmp( lex.buffer.size() + (LexerImpl::BUFFER_SIZE / 2), '\0' );

                    // Move data to new buffer. bufferPointer points to start of the token.
                    std::memmove( &(tmp[0]), lex.bufferPointer, fetchOffset);

                    // Assign the new buffer to the lex.buffer (MOVE, no copies).
                    lex.buffer.assign( std::move( tmp ) );
                }
                else
                    lex.buffer.resize(lex.buffer.size() + (LexerImpl::BUFFER_SIZE / 2), '\0');

                // Mark this flag, for buffer eset after valid token is matched.
                bufferWasExtended = true;
            }
            // If token is not longer than, no resizes needed.
            // Move memory to start of the buffer, if the token starts later.
            else if( fetchOffset > 0 )
                std::memmove( &(lex.buffer[0]), lex.bufferPointer, fetchOffset );

            if( lex.verbosity > 2 ){
                std::cout << " After ReBuffering: New BufLen: "<< lex.buffer.size()
                          << ", token length: "<< fetchOffset <<"\n";  
                if( lex.buffer.size() < 50 ) std::cout<< " Buffer After: "<<lex.buffer<<"\n";
            }
        }

        // Fetch the new data from stream. All pointers will automatically be assigned.
        if( !lex.updateBuffer( fetchOffset ) ){
            if( !bufferWasExtended )
                return;

            // However if the buffer was extended, and no data were extracted,
            // We must ReSet the end of buffer to the point where current data ends.
            if( reBufferNeeded )
                lex.bufferEnd =  &(lex.buffer[0]) + fetchOffset;
        }

        // If buffer was extended (token was too long).
        // the data which needs to be checked starts at the beginning.
        if( reBufferNeeded || bufferWasExtended ){
            lex.bufferPointer = &(lex.buffer[0]);
        }
    }
}
 
/*! Runner function which uses the single-token getter function
 *  (which has a huge overhead).
 *  - Easy and simple implementation.
 *  - For more optimized, dedicated runner iteration loop, use the
 *    runner_dedicatedIteration function.
 */ 
void LexerImpl::runner_usingTokenGetter( LexerImpl& lex ){
    while( 1 ){
        // Allocate a new token.
        LexicToken tok;
        int tt = lex.getNextTokenPriv( lex, tok );

        // If good, put to queue.
        if(tt >= 0)
            lex.bQueue->push( std::move( tok ) ); 

        // Break loop if Fatal Error or Stream Ended (-1).
        if( tt < 0 )
            break; 
    }
}

/*! Public interface to runner function, which fills the blocking queue with tokens.
 *  - Does the job of synchronization (mutexes, etc), 
 *    and calls the implementation function.
 *  - Implementation runs until the end of stream, or until error occurs.
 *
 *  - CALLING RULES:
 *    - Can be called only when multithreading (using the Blocking Queue).
 *    - Also, only one instance of this function may execute at time.
 *
 *  TODO: Don't use blocking queues, but implement an embedded condition-waiting 
 *        system, to avoid overhead and error-pronity of pushing END_OF_STREAM_TOKEN. 
 */
void LexerImpl::start(){
    // Check if call conditions are met (not running already, and using threads).
    if( running || !useBlockingQueue )
        return;
    running = true; 

    // Call implementation-specific private function.
    runnerPriv( *this );
     
    // At the end, push a token which will indicate the end of stream, to prevent deadlocks.
    bQueue->push( LexicToken( LexicToken::END_OF_STREAM_TOKEN, std::string() ) ); 

    running = false;    
}

/*! Get next token. 
 *  - If no multithreading is used, it directly calls the backend function.
 *  - Can return invalid tokens if non-fatal error occured.
 *  @param tok - reference to token to-be-filled.
 *  @return true - if valid token was extracted.
 */ 
bool LexerImpl::getNextToken( LexicToken& tok ){
    // If using multithreading, wait until token gets put into the queue.
    // If stream has ended, but queue has tokens, extract them.
    if( useBlockingQueue ){
        if(endOfStream && bQueue->isEmpty() && !running)
            return false;

        tok = bQueue->pop();
        return tok.id != LexicToken::END_OF_STREAM_TOKEN;
    }
    
    // If no multithreading, just directly call the private token getter.
    int ret = getNextTokenPriv( *this, tok );

    // return true if non-fatal error, and not end of stream.
    return ret >= 0 ;
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

