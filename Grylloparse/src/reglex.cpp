#include "reglex.hpp"
#include <iostream>
#include <map>

namespace gparse{

/*! Special Tag class.
 *  Used to perform actions based on special tags got, 
 *  when constructing RegLex from the GBNF.
 */
class SpecialTag{
private:
    std::string name;
    int type;
    std::function< int( RegLexData&, const gbnf::GbnfData&, int, 
                        const std::string&, int, void* ) > processor;

public:
    // Valid special tag types. The execution time and place depends on the type.
    const static int TYPE_NON_REGEX       = 1;
    const static int TYPE_PROPERTY        = 2;
    const static int TYPE_RECURSIVE_REGEX = 3;

    // Valid processor return values. Sets, e.g. if the rule being 
    // processed needs to be deleted.
    const static int RET_DO_NOTHING         = 0;
    const static int RET_DELETE_REGLEX_RULE = 1;

    // Call conditions.
    const static int COND_CALL_IN_RULE_RESOLVE_LOOP = 1;
    const static int COND_BEFORE_RULE_RESOLVE_LOOP  = 2;
    const static int COND_ALL_RULES_READY = 3;

    SpecialTag( const std::string& _name )
        : name( _name )
    {}
    SpecialTag( const std::string& _name, int _type,
        std::function< int(RegLexData&, const gbnf::GbnfData&, int, 
                       const std::string&, int, void*) >&& proc )
        : name( _name ), 
          type( _type ),
          processor( std::move(proc) )
    {}

    const std::string& getName() const { return name; }
    int getType() const { return type; }

    int process( RegLexData& reg, const gbnf::GbnfData& gdata, int id, 
        const std::string& str, int callConditions, void* param = nullptr ) const {
        return processor( reg, gdata, id, str, callConditions, param );
    }

    bool operator== (const SpecialTag& other) const {
        return name == other.name;
    }

    bool operator< (const SpecialTag& other) const {
        return name < other.name;
    }
     
    bool operator> (const SpecialTag& other) const {
        return name > other.name;
    } 
};

static const std::set< SpecialTag > SpecialTags({
    // The custom whitespaces control tag.
    SpecialTag( "regex_ignore", SpecialTag::TYPE_RECURSIVE_REGEX,
        []( RegLexData& rl, const gbnf::GbnfData& gdata, int id, 
            const std::string& str, int callCondition, void* param ) -> int
        {
            rl.useCustomWhitespaces = true; 
            if( callCondition == SpecialTag::COND_CALL_IN_RULE_RESOLVE_LOOP ){
                rl.regexWhitespaces = RegLexRule( 0, std::regex( str ), str );
                return SpecialTag::RET_DELETE_REGLEX_RULE;
            }
            else{
                auto&& regWsIter = rl.rules.find( RegLexRule( id ) );
                if( regWsIter != rl.rules.end() )
                    rl.regexWhitespaces = *regWsIter; // Assign rule from the one on table.
                else
                    throw std::runtime_error("[RegLexData(GbnfData)]: \
                                <delim_regex> rule is not present."); 
                rl.rules.erase( regWsIter );
            }
            return SpecialTag::RET_DO_NOTHING;
        } ),

    // Non-Regex (Simple string) data type rules.
    // By now, do nothing, because these properties are not yet implemented.
    SpecialTag( "delim", SpecialTag::TYPE_NON_REGEX,
        []( RegLexData& rl, const gbnf::GbnfData&, int id, 
            const std::string& str, int, void* param ) -> int{
            return SpecialTag::RET_DO_NOTHING;
        } ),

    SpecialTag( "ignore", SpecialTag::TYPE_NON_REGEX,
        []( RegLexData& rl, const gbnf::GbnfData&, int id, 
            const std::string& str, int, void* param ) -> int{
            return SpecialTag::RET_DO_NOTHING;
        } )
});

/*! Recursively collects the string segments into one Regex,
 *  traversing the grammar tokens.
 *  @param str - string to which to append all the collected stuff
 *  @param rule - starting GrammarRule,
 *  @param recLevel - level of recursion.
 *  @return true, if successfully collected a regex.
 */ 
static bool collectRegexStringFromGTokens_priv( const gbnf::GbnfData& data,
    std::string& str, const gbnf::GrammarRule& rule, std::set<int>& idStack,
    bool parentMultiOption = false, size_t recLevel = 0 )
{
    // First, check if we haven't came into a recursive loop.
    // Check if we have already processed a rule with an ID of "rule.getID()".
    if( idStack.find( (int)(rule.getID()) ) != idStack.end() ) 
        return false;
    else // Insert current ID as the one being processed.
        idStack.insert( (int)(rule.getID()) );

    // Check if there is only one option, with only one token, of type REGEX.
    // Don't use groups if so. Only Non-Recursive one-level tokens can be made so.
    if( rule.options.size() == 1 && rule.options[0].children.size() == 1 &&
        rule.options[0].children[0].type == gbnf::GrammarToken::REGEX_STRING && 
        !parentMultiOption )
    {
        str += rule.options[0].children[0].data; 
        return true;
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
                           rule.options.size() > 1, recLevel+1 );
                }
            }
        }
    }

    // Regex group end.
    str += ")";

    // At the end, pop current rule's ID from the stack.
    idStack.erase( (int)(rule.getID()) );

    return true;
}

/*! Function checks if assigned GBNF-type grammar is supported.
 *  @throws an exception if grammar contain wrong rules/tokens.
 */ 
static inline void checkAndAssignLexicProperties( RegLexData& rl, 
        const gbnf::GbnfData& gdata, bool useStringRepresentations = true, 
        bool constructIndividualRules = false,
        bool useErrorFallbackRule = true )
{
    // Find the specification declarations.
    std::map< int, const SpecialTag& > nonRegexSpecTags;
    std::map< int, const SpecialTag& > regexSpecTags;
    
    // Tags to be ignored in the regex collection.
    std::set<int> ignoredTags;

    for( auto&& nt : gdata.tagTableConst() ){
        // Check if special tag. If yes, add to the specials map for later processing.
        auto&& specTag = SpecialTags.find( SpecialTag(nt.data) );
        if( specTag != SpecialTags.end() ){
            if( specTag->getType() == SpecialTag::TYPE_NON_REGEX )
                nonRegexSpecTags.insert( std::pair< int, const SpecialTag& >( \
                                         nt.getID(), *specTag ) );
            else if( specTag->getType() == SpecialTag::TYPE_RECURSIVE_REGEX )
                regexSpecTags.insert( std::pair< int, const SpecialTag& >( \
                                                 nt.getID(), *specTag ) ); 
        }
    }

    // --- Set Non-Regex (Simple) special rules --- //
    
    for( auto&& a : nonRegexSpecTags ){
        a.second.process( rl, gdata, a.first, std::string(), 
            SpecialTag::COND_BEFORE_RULE_RESOLVE_LOOP, nullptr );
    }

    // If using delimiters (simple character array), just assign data string.
    // Only STRING type token can define non-regex delimiters.
    /*if( sDelimTag != (size_t)(-1) ){
        auto&& sDelimRule = gdata.getRule( sDelimTag );

        if( sDelimRule != gdata.grammarTableConst().end() &&
            sDelimRule->options.size() == 1 )
        {
            rl.nonRegexDelimiters = sDelimRule->options[0].children[0].data;
            ignoredTags.insert( sDelimTag );
            std::cout<<"Simple Delim Rule: \""<< rl.nonRegexDelimiters <<"\"\n"; 
        }
        else
            throw std::runtime_error("[RegLexData(GbnfData)]: <delim> rule is not present."); 
        rl.tokenized = true;
    }*/

    // If ignorable tag was found, assign ignoreables string.
    // Ignorables can't be regex by nature, it's a list of whitespace chars.
    /*if( ignoreTag != (size_t)(-1) ){
        auto&& ignoreRule = gdata.getRule( ignoreTag );

        if( ignoreRule != gdata.grammarTableConst().end() &&
            ignoreRule->options.size() == 1 )
        {
            rl.whitespaces = ignoreRule->options[0].children[0].data;
            ignoredTags.insert( ignoreTag );
            std::cout<<"Ignoreable Rule: \""<< rl.nonRegexDelimiters <<"\"\n"; 
        }
        else
            throw std::runtime_error("[RegLexData(GbnfData)]: <ignore> rule is not present."); 
        rl.useCustomWhitespaces = true;
    }*/

    // Collect the regexes for each rule.
    // Construct a final regex, which will be used in lexer-tokenizing the language.
    std::string finalRegex;
    finalRegex.reserve( gdata.grammarTableConst().size() * 12 );

    for( auto&& rule : gdata.grammarTableConst() ){
        std::string regstr;
        if( collectRegexStringFromGTokens_priv( gdata, regstr, rule, ignoredTags ) ){
            // Check if current ID is of a special tag. If so, perform operations.
            auto&& specTag = regexSpecTags.find( rule.getID() );
            if( specTag != regexSpecTags.end() ){
                if( specTag->second.process(rl, gdata, rule.getID(), regstr, 
                    SpecialTag::COND_CALL_IN_RULE_RESOLVE_LOOP, nullptr ) 
                    == SpecialTag::RET_DELETE_REGLEX_RULE )
                    continue;
            }

            // Add current regex to the Final Regex String.
            if( !constructIndividualRules ){
                finalRegex.append( "(" );
                finalRegex.append( std::move(regstr) );
                finalRegex.append( ")|" );
            }

            // Add the rule, if specified.
            else{ // constructIndividualRules 
                finalRegex += "(" + regstr + ")|";

                if( !useStringRepresentations )
                    rl.rules.insert(RegLexRule(rule.getID(), std::regex(std::move(regstr))));
                else
                    rl.rules.insert( RegLexRule( rule.getID(), std::regex( regstr ),  \
                                                 std::move(regstr) ) );
            }

            // Add current ID to the ID map.
            rl.tokenTypeIDs.push_back( rule.getID() );
        }
    }

    // Complete the full regex.
    // Add a whitespace rule's capt. group. If we use custom whitespaces, add them.
    // If standard WS, use the \s regex.
    finalRegex.push_back( '(' );
    if( rl.useCustomWhitespaces )
        finalRegex.append( rl.regexWhitespaces.stringRepr );
    else
        finalRegex.append( "\\s+" ); 
    finalRegex.push_back( ')' );

    rl.spaceRuleIndex = rl.tokenTypeIDs.size();

    // If using error fallback rule, add an additional group for catching everything else.
    if( useErrorFallbackRule ){
        finalRegex.append( "|(.+)" );
        rl.errorRuleIndex = rl.spaceRuleIndex + 1;
    }

    // Assign the final full regex.
    rl.fullLanguageRegex.regex = std::regex( finalRegex );
    rl.fullLanguageRegex.stringRepr = std::move( finalRegex );
}

/*! RegLexData constructor from GBNF grammar.
 */ 
RegLexData::RegLexData( const gbnf::GbnfData& data, bool useStringReprs ){
    checkAndAssignLexicProperties( *this, data, useStringReprs ); 
}

void RegLexData::print( std::ostream& os ) const {
    os << "RegLexData:\n";
    // Bools
    os << " useCustomWhitespaces: "<< useCustomWhitespaces <<"\n"; 
    os << " useFallbackErrorRule: "<< useFallbackErrorRule <<"\n"; 
    os << " spaceRuleIndex: "<< spaceRuleIndex <<"\n"; 
    os << " errorRuleIndex: "<< errorRuleIndex <<"\n"; 

    // Actual properties.
    if( useCustomWhitespaces )
        os << " regexWhitespaces: "<< regexWhitespaces.stringRepr <<"\n"; 

    if( fullLanguageRegex.stringRepr.size() > 100 )
        os <<" fullLanguageRegex: "<< fullLanguageRegex.stringRepr.size() <<" chars.\n";
    else
        os <<" fullLanguageRegex: "<< fullLanguageRegex.stringRepr <<"\n";

    if( !tokenTypeIDs.empty() ){
        os <<" Final regex group ID Map: \n  ";
        for( size_t i = 0; i < tokenTypeIDs.size(); i++ ){
            os << "["<< i <<" -> "<< tokenTypeIDs[ i ] <<"] ";
        }
        os <<"\n";
    }
     
    if(!rules.empty()){
        os<<" Rules:\n  ";
        for( auto&& a : rules ){
            os << a.getID() <<" -> "<< a.stringRepr <<"\n  ";
        }
    }
}

}

