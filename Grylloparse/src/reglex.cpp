#include "reglex.hpp"

namespace gparse{

/*! Recursively collects the string segments into one Regex,
 *  traversing the grammar tokens.
 *  @param str - string to which to append all the collected stuff
 *  @param rule - starting GrammarRule,
 *  @param recLevel - level of recursion.
 */ 
static void collectRegexStringFromGTokens_priv( const gbnf::GbnfData& data,
    std::string& str, const gbnf::GrammarRule& rule, std::set<int>& idStack,
    bool parentMultiOption = false )
{
    // First, check if we haven't came into a recursive loop.
    // Check if we have already processed a rule with an ID of "rule.getID()".
    if( idStack.find( (int)(rule.getID()) ) != idStack.end() ) 
        return;
    else // Insert current ID as the one being processed.
        idStack.insert( (int)(rule.getID()) );

    // Check if there is only one option, with only one token, of type REGEX.
    // Don't use groups if so. Only Non-Recursive one-level tokens can be made so.
    if( rule.options.size() == 1 && rule.options.children.size() == 1 &&
        rule.options.children[0].type == gbnf::GrammarToken::REGEX_STRING && 
        idStack.empty() && !parentMultiOption )
    {
        str += rule.options.children[0].data; 
        return;
    }

    // Regex group start - use Non-Capturing Groups.
    str += "(?:";

    // Loop all options of the rule and construct a regex.
    bool first = true;
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
                auto&& iter = data.getRule( token.id );
                if( iter != data.grammarTableConst().end() ){
                    collectRegexStringFromGTokens_priv( data, str, *iter, idStack,
                           rule.options.size() > 1 );
                }
            }
        }
    }

    // Regex group end.
    str += ")";

    // At the end, pop current rule's ID from the stack.
    idStack.erase( (int)(rule.getID()) );
}

static void collectRegexStringFromGTokens( const gbnf::GbnfData& data,
                        std::string& str, const auto& rule ){
    std::set<int> stack;
    collectRegexStringFromGTokens_priv( data, str, rule, stack );
}

/*! Function checks if assigned GBNF-type grammar is supported.
 *  @throws an exception if grammar contain wrong rules/tokens.
 */ 
static inline void checkAndAssignLexicProperties( 
        RegLexData& rl, const gbnf::GbnfData& gdata )
{
    // Find if delimiter is there.
    size_t regexDelimTag = (size_t)(-1);
    size_t sDelimTag = (size_t)(-1);
    for( auto&& nt : gdata.tagTableConst() ){
        if( nt.data == "regex-delim" ){
            regexDelimTag = nt.getID();
            break;
        }
        if( nt.data == "delim" ){
            sDelimTag = nt.getID();
            break;
        } 
    }
    
    // If delim tag not found, we must throw an error.
    // TODO: Use No-Delimiting parser mode if no <delim> tag.
    if( (regexDelimTag == (size_t)(-1) && sDelimTag == (size_t)(-1)) || 
        (regexDelimTag != (size_t)(-1) && sDelimTag != (size_t)(-1)) )
        throw std::runtime_error("[RegLexData(GbnfData)]: "
                "<delim> tags are not found or wrongly defined.");

    rl.tokenized = true;
    rl.useRegexDelimiters = (regexDelimTag != (size_t)(-1));

    // If using non-regex delimiters (simple characters), just assign data string.
    // Only STRING type token can define non-regex delimiters.
    if( !rl.useRegexDelimiters ){
        auto&& sDelimRule = gdata.getRule( sDelimTag );

        if( sDelimRule != gdata.grammarTableConst().end() )
            rl.nonRegexDelimiters = sDelimRule->options[0].children[0].data;
        else
            throw std::runtime_error("[RegLexData(GbnfData)]: <delim> rule is not present."); 
    }

    // Collect the regexes for each rule.
    for( auto&& rule : gdata.grammarTableConst() ){
        std::string regstr;
        collectRegexStringFromGTokens( gdata, regstr, rule );

        rl.rules.insert( RegLexRule(rule.getID(), std::regex( regstr )) );
    }

    if( rl.useRegexDelimiters ){
        auto&& regDelIter = rl.rules.find( RegLexRule(regexDelimTag) );
        if( regDelIter != rl.rules.end() )
            rl.regexDelimiters = regDelIter->regex;
        else
            throw std::runtime_error("[RegLexData(GbnfData)]: <delim-regex> rule is not present."); 
    }
}


/*! RegLexData constructor from GBNF grammar.
 */ 
RegLexData::RegLexData( const gbnf::GbnfData& data ){
    checkAndAssignLexicProperties( *this, data ); 
}

}

