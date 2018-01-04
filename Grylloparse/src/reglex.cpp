#include "reglex.hpp"
#include <iostream>

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
    if( rule.options.size() == 1 && rule.options[0].children.size() == 1 &&
        rule.options[0].children[0].type == gbnf::GrammarToken::REGEX_STRING && 
        idStack.empty() && !parentMultiOption )
    {
        str += rule.options[0].children[0].data; 
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
static inline void checkAndAssignLexicProperties( RegLexData& rl, 
        const gbnf::GbnfData& gdata, bool useStringRepresentations )
{
    // Find delimiters and ignorables
    size_t regexDelimTag = (size_t)(-1);
    size_t sDelimTag = (size_t)(-1);
    size_t ignoreTag = (size_t)(-1);

    for( auto&& nt : gdata.tagTableConst() ){
        if( nt.data == "regex_delim" ){
            regexDelimTag = nt.getID();
        }
        if( nt.data == "delim" ){
            sDelimTag = nt.getID();
        } 
        if( nt.data == "ignore" ){
            ignoreTag = nt.getID();
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

        if( sDelimRule != gdata.grammarTableConst().end() &&
            sDelimRule->options.size() == 1 )
        {
            rl.nonRegexDelimiters = sDelimRule->options[0].children[0].data;
            std::cout<<"Non-Regex Delim Rule: \""<< rl.nonRegexDelimiters <<"\"\n"; 
        }
        else
            throw std::runtime_error("[RegLexData(GbnfData)]: <delim> rule is not present."); 
    }

    // If ignorable tag was found, assign ignoreables string.
    // Ignorables can't be regex by nature, it's a list of whitespace chars.
    if( ignoreTag != (size_t)(-1) ){
        auto&& ignoreRule = gdata.getRule( ignoreTag );

        if( ignoreRule != gdata.grammarTableConst().end() &&
            ignoreRule->options.size() == 1 )
        {
            rl.ignorables = ignoreRule->options[0].children[0].data;
            std::cout<<"Ignoreable Rule: \""<< rl.nonRegexDelimiters <<"\"\n"; 
        }
        else
            throw std::runtime_error("[RegLexData(GbnfData)]: <ignore> rule is not present."); 
        rl.useCustomWhitespaces = true;
    }

    // Collect the regexes for each rule.
    for( auto&& rule : gdata.grammarTableConst() ){
        std::string regstr;
        collectRegexStringFromGTokens( gdata, regstr, rule );

        if( !useStringRepresentations )
            rl.rules.insert( RegLexRule(rule.getID(), std::regex( std::move(regstr) )) );
        else
            rl.rules.insert( RegLexRule( rule.getID(), std::regex( regstr ), 
                                         std::move(regstr) ) );
    }

    // Find the regex delimiter rule.
    if( rl.useRegexDelimiters ){
        auto&& regDelIter = rl.rules.find( RegLexRule(regexDelimTag) );
        if( regDelIter != rl.rules.end() )
            rl.regexDelimiters = *regDelIter;
        else
            throw std::runtime_error("[RegLexData(GbnfData)]: <delim-regex> rule is not present."); 
    }

    // Removes rules which define special data (delimiters, ignorables).
    if(regexDelimTag != (size_t)(-1))
        rl.rules.erase( RegLexRule(regexDelimTag) );

    if(sDelimTag != (size_t)(-1))
        rl.rules.erase( RegLexRule(sDelimTag) );

    if(ignoreTag != (size_t)(-1))
        rl.rules.erase( RegLexRule(ignoreTag) );
}

/*! RegLexData constructor from GBNF grammar.
 */ 
RegLexData::RegLexData( const gbnf::GbnfData& data, bool useStringReprs ){
    checkAndAssignLexicProperties( *this, data, useStringReprs ); 
}

void RegLexData::print( std::ostream& os ) const {
    os << "RegLexData:\n";
    if(!nonRegexDelimiters.empty())
        os << " nonRegexDelimiters: "<< nonRegexDelimiters <<"\n";
    if(!ignorables.empty())
        os << " ignorables: "<< ignorables <<"\n";
    if(useRegexDelimiters || !regexDelimiters.regexStringRepr.empty())
        os << " regexDelimiters: "<< regexDelimiters.regexStringRepr <<"\n";
    if(!rules.empty()){
        os<<" Rules:\n  ";
        for( auto&& a : rules ){
            os << a.getID() <<" -> "<< a.regexStringRepr <<"\n  ";
        }
    }
}

}

