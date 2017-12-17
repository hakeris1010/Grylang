#ifndef REGLEX_HPP_INCLUDED
#define REGLEX_HPP_INCLUDED

#include <string>
#include <regex>
#include <set>
#include <gbnf/gbnf.hpp>

namespace gparse{

/*! Regex-based Lexic Grammar format - RegLex
 * - Simple rules are defined only as regexes.
 * - Various formats may be supported.  
 */ 

struct RegLexRule{
    int id;
    std::regex regex;

    RegLexRule();
    RegLexRule(int _id, const std::regex& _reg) : id( _id ), regex( _reg ) {}
    RegLexRule(int _id, std::regex&& _reg) : id( _id ), regex( std::move(_reg) ) {}

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

    // If tokenized and not using regex delimiters, 
    // delimiters are chars of this string.
    std::string nonRegexDelimiters;

    // Otherwise, regex defines delimiters.
    std::regex regexDelimiters;

    // Language Lexics properties.
    bool useRegexDelimiters = true; 
    bool tokenized = true;

    /*! Full-data constructors.
     */ 
    RegLexData();
    RegLexData( std::initializer_list< RegLexRule >&& _rules, 
                bool useRegDels = true,
                const std::string& nonRegDels = std::string(),
                const std::regex& regDels = std::regex() )
        : rules( std::move(_rules) ), nonRegexDelimiters( nonRegDels ),
          regexDelimiters( regDels ), useRegexDelimiters( useRegDels ),
          tokenized( useRegDels || !nonRegDels.empty() )
    {}

    RegLexData( std::initializer_list< RegLexRule >&& _rules, 
                bool useRegDels = true,
                std::string&& nonRegDels = std::string(),
                std::regex&& regDels = std::regex() )
        : rules( std::move(_rules) ), nonRegexDelimiters( std::move(nonRegDels) ),
          regexDelimiters( std::move(regDels) ), useRegexDelimiters( useRegDels ),
          tokenized( useRegDels || !nonRegDels.empty() )
    {} 

    /*! Generator constructor - GBNF-style to RegLex.
     *  - Inputs a language lexicon - defining GBNF grammar,
     *    and converts in to RegLex grammar.
     *  - Deduces the tokenizability and type of grammar from specific tags.
     *  - Every tag-rule will be converted to a regex.
     */ 
    RegLexData( const gbnf::GbnfData& data );
};

}

#endif // REGLEX_HPP_INCLUDED 

