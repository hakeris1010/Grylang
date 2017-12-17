#include "reglex.hpp"

RegLexData::RegLexData( const gbnf::GbnfData& data ){
    checkAndAssignLexicProperties( *this, data ); 
}

/*! Function checks if assigned GBNF-type grammar is supported.
 *  @throws an exception if grammar contain wrong rules/tokens.
 */ 
static inline void checkAndAssignLexicProperties( 
        RegLexData& rl, const gbnf::GbnfData& gdata )
{
    // Find if delimiter is there.
    short regexDelimTag = -1;
    short sDelimTag = -1;
    for( auto&& nt : gdata.tagTable ){
        if( nt.data == "regex-delim" ){
            regexDelimTag = nt.ID;
            break;
        }
        if( nt.data == "delim" ){
            sDelimTag = nt.ID;
            break;
        } 
    }
    
    // If delim tag not found, we must throw an error.
    // TODO: Use No-Delimiting parser mode if no <delim> tag.
    if( (regexDelimTag == -1 && sDelimTag == -1) || 
        (regexDelimTag != -1 && sDelimTag != -1) )
        throw std::runtime_error("[RegLexData(GbnfData)]: "
                "<delim> tags are not found or wrongly defined.");

    rl.tokenized = true;
    rl.useRegexDelimiters = (regexDelimTag != -1);

    // If using non-regex delimiters (simple characters), just assign data string.
    // Only STRING type token can define non-regex delimiters.
    if( !rl.useRegexDelimiters ){
        auto&& delimIter = gdata.grammarTable.find( sDelimTag );
        if( delimIter == gdata.grammarTable.end() )
            throw std::runtime_error("[RegLexData(GbnfData)]: <delim> rule is not present."); 

        rl.nonRegexDelimiters = delimIter->options[0].children[0].data;
    }

    // Collect the regexes for each rule.
    for( auto&& rule : gdata.grammarTable ){
        std::string regstr;
        collectRegexStringFromGTokens( gdata, regstr, rule );

        rl.rules.insert( RegLexRule(rule.ID, std::regex( regstr )) );
    }

    if( rl.useRegexDelimiters )
        rl.regexDelimiters = rl.rules.find( regexDelimTag );
}

/*! Recursively collects the string segments into one Regex,
 *  traversing the grammar tokens.
 *  @param str - string to which to append all the collected stuff
 *  @param rule - starting GrammarRule,
 *  @param recLevel - level of recursion.
 */ 
static inline void collectRegexStringFromGTokens( const GbnfData& data,
                std::string& str, const auto& rule, int recLevel = 0 )
{
    bool first = true;

    // Regex group start
    str += "(?:";

    for( auto&& opt : delimRule.options ){
        auto&& token = opt.children[0];

        // Add OR if not first option
        if(!first)
            str += "|";
        else
            first = false;

        // Now check the types of tokens, and concatenate regex.
        if( token.type == gbnf::GrammarToken::REGEX_STRING )
            str += token.data;

        else if( token.type == gbnf::GrammarToken::TAG_ID ){
            // Find rule which defines this tag, and launch a collector on that rule.
            auto&& iter = lexics.grammarTable.find( token.id );
            if( delimIter == lexics.grammarTable.end() )
                throwError("[collectRegexString]: ["+ std::to_string(token.id) +
                           "] rule is not present in table.");     
        }
    }

    // Regex group end.
    str += ")";
}


