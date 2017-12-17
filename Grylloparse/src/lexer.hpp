#ifndef LEXER_HPP_INCLUDED
#define LEXER_HPP_INCLUDED

#include <memory>
#include <string>
#include <gbnf/gbnf.hpp>

namespace gparse{

/*! Token structure.
 *  Includes type and data.
 *  @property type - the GBNF Grammar-defined type of the token
 *  @data - an actual string data of the token.
 */     
struct LexicToken{
    int id;
    std::string data;    

    LexicToken(){}
    LexicToken( int _id, std::string&& _data ) : id( _id ), data( _data ) {}
    LexicToken( int _id, const std::string& _data ) : id( _id ), data( _data ) {}
};

inline std::ostream& operator<<( std::ostream& os, const LexicToken& tok ){
    os<<"id: "<< tok.id <<", data: \""<< tok.data << "\"";
    return os;
}

/*! Base lexer class.
 * Other lexers (implementations and a public lexer) derive from this.
 */ 
class BaseLexer{
public:
    virtual ~BaseLexer(){}

    virtual void start() = 0;
    virtual bool getNextToken( LexicToken& tok ) = 0;
};

/*! Lexical parsing class.
 *  - Tokenizes the stream by given lexical grammar data.
 *  - Is fully thread-safe, and uses blocking queues for data sharing.
 *  - By now, tokenizing is done only by using regexes.
 *
 *  Supported GBNF Lexics:
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
class Lexer : public BaseLexer{
private:
    std::unique_ptr< BaseLexer > impl;

public:
    /*! Constructors.
     *  @param lexicData - a GBNF-style grammar form data.
     *      - Only REGEX_STRING type rules will be used.
     *  @param stream - a stream which to parse data from.
     *  @param useBlockingQueue - if set, tokens will be put 
     *         into a thread-safe blocking queue.
     */ 
    Lexer( const gbnf::GbnfData& lexicData, std::istream& stream, bool useBlockingQueue = false );
    Lexer( gbnf::GbnfData&& lexicData, std::istream& stream, bool useBlockingQueue = false );
    ~Lexer(){}

    /*! Starts parsing tokens from stream to the queue.
     *  - Works only if useBlockingQueue param is specified on construction.
     */ 
    void start();

    /*! Returns next token from a stream.
     *  - If useBlockingQueue is set, it's called from a different thread.
     *    - It waits on a condition variable until a token has been put to queue.
     *      and returns it.
     *  - If no multithreading, it just parses a new token from a stream.
     *  @param tok - a reference to a token to fill.
     *  @return true if there are more tokens to read.
     */ 
    bool getNextToken( LexicToken& tok );
};

/*! Automated lexical parser class.
 *  Used only by the parser generator.
 *  All rules are hard-coded.
 */  
class AutoLexer : public BaseLexer{
public:
    AutoLexer( std::istream& strm );
    virtual ~AutoLexer();
    
    bool getNextToken( LexicToken& tok );
};

}

#endif // LEXER_HPP_INCLUDED

