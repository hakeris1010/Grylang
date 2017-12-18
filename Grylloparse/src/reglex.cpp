#include "reglex.hpp"

namespace gparse{

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

static void collectRegexStringFromGTokens( const gbnf::GbnfData& data,
                        std::string& str, const auto& rule ){
    std::set<int> stack;
    collectRegexStringFromGTokens_priv( data, str, rule, stack );
}

/*! Recursively collects the string segments into one Regex,
 *  traversing the grammar tokens.
 *  @param str - string to which to append all the collected stuff
 *  @param rule - starting GrammarRule,
 *  @param recLevel - level of recursion.
 */ 
static void collectRegexStringFromGTokens_priv( const gbnf::GbnfData& data,
    std::string& str, const gbnf::GrammarRule& rule, std::set<int>& idStack )
{
    // First, check if we haven't came into a recursive loop.
    // Check if we have already processed a rule with an ID of "rule.ID".
    if( idStack.find( (int)(rule.ID) ) != idStack.end() ) 
        return;
    else // Insert current ID as the one being processed.
        idStack.insert( (int)(rule.ID) );

    bool first = true;

    // Regex group start - use Non-Capturing Groups.
    str += "(?:";

    // Loop all options of the rule and construct a regex.
    for( auto&& opt : rule.options ){
        // Add OR if not first option
        if(!first)
            str += "|";
        else
            first = false;

        // Loop all tokens of this option.
        for( auto&& token : opt.children ){
            // Now check the types of tokens, and concatenate regex.
            if( token.type == gbnf::GrammarToken::REGEX_STRING )
                str += token.data;

            else if( token.type == gbnf::GrammarToken::TAG_ID ){
                // Find rule which defines this tag, and launch a collector on that rule.
                auto&& iter = data.grammarTable.find( token.id );
                if( iter != data.grammarTable.end() ){
                    collectRegexStringFromGTokens_priv( data, str, *iter, idStack );
                }
            }
        }
    }

    // Regex group end.
    str += ")";

    // At the end, pop current rule's ID from the stack.
    idStack.erase( (int)(rule.ID) );
}

/*! RegLexData constructor from GBNF grammar.
 */ 
RegLexData::RegLexData( const gbnf::GbnfData& data ){
    checkAndAssignLexicProperties( *this, data ); 
}

}

