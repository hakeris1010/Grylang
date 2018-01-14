#ifndef REGLEX_HPP_INCLUDED
#define REGLEX_HPP_INCLUDED

#include <string>
#include <regex>
#include <set>
#include <map>
#include <ostream>
#include <gbnf/gbnf.hpp>

namespace gparse{

/*! Regex-based Lexic Grammar format - RegLex
 * - Simple rules are defined only as regexes.
 * - Various formats may be supported.  
 */ 

struct RegLexRule{
private:
    bool ready = false;
    size_t id; 

public:
    std::regex regex;
    std::string stringRepr;

    RegLexRule(){}
    RegLexRule(size_t _id, const std::regex& _reg = std::regex(), 
               const std::string& _stringRepr = std::string() ) 
        : ready( true ), id( _id ), regex( _reg ), stringRepr( _stringRepr ) 
    {}
    RegLexRule(size_t _id, std::regex&& _reg, std::string&& _stringRepr = std::string()) 
        : ready( true ), id( _id ), regex( std::move(_reg) ), 
          stringRepr( std::move( _stringRepr ) ) 
    {}

    size_t getID() const { return id; }
    bool isReady() const { return ready; }
    void setReady( bool val = true ){ ready = val; }

    bool operator< (const RegLexRule& other) const {
        return id < other.id;
    }
};

/*! RegLex Data structure.
 *  - Contains all properties necessary to parse lexic tokens.
 *  - Can be generated from GBNF grammar.
 *
 *  Supported GBNF grammar must be:
 *
 *  - Must contain a <delim> rule, which defines the delimiters to use when tokenizing.
 *
 *  - The lexem-defining rule options must be only of REGEX_STRING type or
 *    tags, which reference other REGEX_STRING rules.
 *
 *  - NO RECURSION ALLOWED.
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
struct RegLexData{
    // The regexes which define tokens.
    std::set< RegLexRule > rules;

    // Final Regex which encompasses all the rules.
    RegLexRule fullLanguageRegex;

    // Array which maps regex group indexes in the final regex,
    // to corresponding Token Type IDs.
    std::vector<int> tokenTypeIDs;
    
    // Custom whitespace (ignoreable) characters, Regex-Type.
    RegLexRule regexWhitespaces;

    // Language Lexics properties.
    bool useCustomWhitespaces = false;
    bool useFallbackErrorRule = true;

    size_t errorRuleIndex;
    size_t spaceRuleIndex;

    /*! Full-data constructors.
     */ 
    RegLexData();
    RegLexData( std::initializer_list< RegLexRule >&& _rules, 
                RegLexRule&& fullRegex            = RegLexRule(),
                std::vector<int>&& tokTypeIdMap   = std::vector<int>(),
                RegLexRule&& regCustomWhitespaces = RegLexRule(),
                bool _useFallbackErrorRule = true )
          : rules( std::move( _rules ) ), 
            fullLanguageRegex( std::move( fullRegex ) ),
            tokenTypeIDs( std::move( tokTypeIdMap ) ),
            regexWhitespaces( std::move( regCustomWhitespaces ) ),
            useCustomWhitespaces( regexWhitespaces.isReady() ),
            useFallbackErrorRule( _useFallbackErrorRule )
    {}
                
    /*! Generator constructor - GBNF-style to RegLex.
     *  - Inputs a language lexicon - defining GBNF grammar,
     *    and converts in to RegLex grammar.
     *  - Deduces the tokenizability and type of grammar from specific tags.
     *  - Every tag-rule will be converted to a regex.
     */ 
    RegLexData( const gbnf::GbnfData& data, bool useStringReprs = false );

    void print( std::ostream& os ) const;
};

inline std::ostream& operator<<( std::ostream& os, const RegLexData& data ){
    data.print( os );
    return os;
}

}

#endif // REGLEX_HPP_INCLUDED 

