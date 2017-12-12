#include "gbnfconverter.hpp"

namespace gbnf::Converter{

class ConverterToBNF{
private:
    GbnfData& data;
    std::vector< GrammarRule > newRules;
    const bool preferRightRecursion = true; // For LR Pars0rz

public:
    ConverterToBNF( GbnfData& _data, _preferRightRec = true ) 
        : data( _data ), preferRightRecursion( _preferRightRec ) {}

    void convert();

    GrammarToken createNewRuleAndGetTag( GrammarToken&& rootToken, int recLevel = 0 );

    //void getRuleFromToken( const GrammarToken& tok, std::vector<GrammarRule>& rules );
    //std::vector<GrammarRule> convertRuleToBnfRules( const GrammarRule& rule );
}

static void getRuleFromToken( const GrammarToken& tok, std::vector<GrammarRule>& rules ){
    GrammarRule* crule = nullptr;

    // Check current token's type. If not BNF, create a new rule to hold it!.
    if( token.type != GrammarToken::TAG_ID && 
        token.type != GrammarToken::REGEX_STRING &&
        token.type != GrammarToken::ROOT_TOKEN )
    {
        data.lastTagID++;
        data.tagTable.insert( NonTerminal( data.lastTagID, 
            "__tmp_convert_to_bnf_"+std::to_string(data.lastTagID) ) 
        );
    }

    bool fixNeeded = false;

    for( for i = 0; i <  ){
        // Check first-level tokens of the current option.
        if( token.type != GrammarToken::TAG_ID && 
            token.type != GrammarToken::REGEX_STRING )
        {    
            if( !fixNeeded ){
                currentRule = std::make_shared< GrammarRule >
            }

            fixNeeded = true;
        }
    }
}

static std::vector<GrammarRule> convertRuleToBnfRules( const GrammarRule& rule ){
    // Loop through all the options of the rule, and check every token of every option, if it complies.
    for( const auto& option : rule.options ){
        
        if( needsFixing )
            break; 
    }
}

/*! Converts a container-type token to a new grammar rule.
 * - Puts new rule to the newRules vector
 *  @param token - a group-type grammar token containing tokens to be parsed.
 *  @param recLevel - level of recursion. Default Zero.
 *  @return a tag-type grammar token referring to newly created rule.
 */ 
GrammarToken ConverterToBNF::createNewRuleAndGetTag( GrammarToken&& token, int recLevel ){
    GrammarToken tagToken;

}

void ConverterToBNF::convert(){
    // Search for rules which contain eBNF tokens.
    for( int i = 0; i < data.grammarTable.size(); i++ ){
        auto& rule = data.grammarTable[ i ];

        // Loop through all the options of the rule, and 
        // check every token of every option, if it hasn't got more layers.
        for( auto& option : rule.options ){
            // Check first-level tokens of the current option.
            for( int i = 0; i < option.children.size(); i++ ){
                auto&& token = option.children[ i ];
                auto type = token.type;

                // Check if it's illegal ( A group ). If so, it needs replacement.
                if( type != GrammarToken::TAG_ID && 
                    type != GrammarToken::REGEX_STRING )
                {
                    // If only one child, and it's a leaf, just use it as repl. However, if
                    // there are more than one, or if it's a tree, create a separate rule. 
                    auto&& replacement = ( 
                        ( token.children.size() == 1 && 
                          token.children[ 0 ].children.empty() ) ?
                        std::move( token.children[0] ) :
                        std::move( createNewRuleAndGetTag( std::move( token ) ) )
                    );

                    // Erase current token, and replace it according to the type.
                    option.children.erase( i, 1 );

                    // Now create new options according to the type.
                    // If a simple grouped elements, just insert at current position.
                    if( type == GROUP_ONE || type == GROUP_REPEAT_ONE )
                        option.insert( i, std::move( replacement ) );
                    // If repeatable or optional, create a new option.
                    else if( type == GROUP_OPTIONAL || type == GROUP_REPEAT_NONE ){
                        i--;
                        // Copy current option, and insert a new token.
                        GrammarToken newOption = option;
                        newOption.insert( i, std::move( replacement ) );
                    }
                }
            }
        }
    }

    // At the end, append newly constructed rules to grammar table.
    data.grammarTable.append( std::move( newRules ) );

}

void removeLeftRecursion( GbnfData& data ){

}

void fullyFixGBNF( GbnfData& data ){

} 

}

