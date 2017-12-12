#include "gbnfconverter.hpp"

namespace gbnf::Converter{

class ConverterToBNF{
private:
    GbnfData& data;
    std::vector< GrammarRule > newRules;
    const bool preferRightRecursion = true; // For LR Pars0rz

public:
    ConverterToBNF( GbnfData& _data, bool _preferRightRec = true ) 
        : data( _data ), preferRightRecursion( _preferRightRec ) {}

    void convert();

    short createNewRuleAndGetTag( GrammarToken&& rootToken, int recLevel = 0 );
    void fixNonBNFTokensInRule( GrammarRule& rule, int recLevel );
};


/*! Converts a container-type token to a new grammar rule.
 * - Puts new rule to the newRules vector
 *  @param token - a group-type grammar token containing tokens to be parsed.
 *  @param recLevel - level of recursion. Default Zero.
 *  @return an ID of the NonTerminal which the newly created rule defines.
 */ 
short ConverterToBNF::createNewRuleAndGetTag( GrammarToken&& token, int recLevel ){
    data.insertNewTag( "__tmp_bnfmode_"+std::to_string( data.getlastTagID() + 1 ) );

    // Backup current token's type
    auto tokType = token.type;

    // Create a new rule to be added to newRules.
    // Assign the token's children as new rule's Option no.1 .
    /*GrammarRule nrule (
        data.lastTagID, { 
            GrammarToken( 
                GrammarToken::ROOT_TOKEN, 0, std::string(),
                std::move( token.children ) 
            )
        }
    );*/

    GrammarRule

    // Binary Recursion !
    // Fix our new rule's options with "fixNonBNFTokensInRule".
    fixNonBNFTokensInRule( nrule, recLevel + 1 );

    // If original token, which we are currently fixing, is of repeatable type,
    // Add current rule's tag as an option, to support recursive repeat.
    if( tokType == GrammarToken::GROUP_REPEAT_NONE ||
        tokType == GrammarToken::GROUP_REPEAT_ONE )
    {
        // Check if only one option is left in the rule. If not, make another rule.
        if( nrule.options.size() > 1 ){
            data.insertNewTag( "__tmp_bnfmode_"+std::to_string( data.getlastTagID() + 1 ) );

            
        }

        nrule.options.insert(
            ( preferRightRecursion ? options.end() : options.begin() ),
            std::move( 
                GrammarToken( GrammarToken::TAG_ID, nrule.ID, std::string(), {} )
            )
        );
    }

    // Push (move) this rule to newRules vector.
    this->newRules.push_back( std::move( nrule ) );

    // Return ID of this rule.
    return nrule.ID;
}

void ConverterToBNF::fixNonBNFTokensInRule( GrammarRule& rule, int recLevel ){
    // Loop through all the options of the rule, and 
    // check every token of every option, if it hasn't got more layers.
    const size_t optSize = rule.options.size();

    for( size_t oi = 0; oi < optSize; oi++ ){
        // Lvalue reference, because we don't want to move it. 
        auto& option = rule.options[ oi ]; 

        // Check first-level tokens of the current option.
        for( size_t i = 0; i < option.children.size(); i++ ){
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
                    // If only one child and it's a non-group type:
                    token.children[0] :
                    // If many childs, and/or it/they have group types:
                    GrammarToken( 
                        GrammarToken::TAG_ID, 
                        createNewRuleAndGetTag( std::move( token ) ),
                        "", {} 
                    )   
                );

                // Now create new options according to the type.
                // Replace current group-type element with generated tag-type 
                // element, referring to the newly created rule or element.
                option.children[ i ] = std::move( replacement );

                // If optional/repeatable, create a new option without current element.
                if( type == GrammarToken::GROUP_OPTIONAL || 
                    type == GrammarToken::GROUP_REPEAT_NONE )
                {
                    // Copy current option, but without a current element.
                    GrammarToken newOption( option );
                    newOption.children.erase( newOption.children.begin() + i );

                    // Push this new option to our rule.
                    rule.options.push_back( std::move( newOption ) );
                }
            }
        }
    }
}

void ConverterToBNF::convert(){
    // Search for rules which contain eBNF tokens.
    for( auto& rule : data.grammarTable ){
        
    }

    // At the end, append newly constructed rules to grammar table.
    data.grammarTable.insert( 
          data.grammarTable.end(),
          newRules.begin(), newRules.end() 
    );
}


/*void removeLeftRecursion( GbnfData& data ){

}

void fullyFixGBNF( GbnfData& data ){

} */

}

