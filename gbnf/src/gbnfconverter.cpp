#include "gbnfconverter.hpp"

namespace gbnf::Converter{

class ConverterToBNF{
private:
    GbnfData& data;

public:
    ConverterToBNF( GbnfData& _data ) : data( _data ) {}
    void convert();

    void getRuleFromToken( const GrammarToken& tok, std::vector<GrammarRule>& rules );
    std::vector<GrammarRule> convertRuleToBnfRules( const GrammarRule& rule );
}

static void getRuleFromToken( const GrammarToken& tok, std::vector<GrammarRule>& rules ){
    GrammarRule* crule = nullptr;
    // Check current token's type. If not BNF, create a new rule to hold it!.
    if( token.type != GrammarToken::TAG_ID && 
        token.type != GrammarToken::REGEX_STRING &&
        token.type != GrammarToken::ROOT_TOKEN )
    {
        data.tagTable.insert( NonTerminal( data.lastTagID, 
            "__tmp_convert_to_bnf_"+std::to_string(data.lastTagID) ) 
        );
        rules.push_back( GrammarRule( 
    }

    bool fixNeeded = false;

    for( auto& token : option.children ){
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

void convertToSimpleBNF( GbnfData& data ){
    // Search for rules which contain eBNF tokens.
    /*for( int i = 0; i < data.grammarTable.size(); i++ ){
        const auto& rule = data.grammarTable[ i ];
        bool needsFixing = false;

        // Loop through all the options of the rule, and check every token of every option, if it complies.
        for( auto& option : rule.options ){
            for( auto& token : option.children ){
                // Check first-level tokens of the current option.
                if( token.type != GrammarToken::TAG_ID && 
                    token.type != GrammarToken::REGEX_STRING ){
                    needsFixing = true;
                    break;
                }
            }
            if( needsFixing )
                break; 
        }

        if( needsFixing ){
            // Get vector of BNF nodes from current eBNF node.
            auto&& bnfRuleVec = convertRuleToBnfRules( rule );

            // Remove the i'th node from vector, and replace it with newly got BNF rules.
            data.grammarTable.erase( i, 1 );
            data.grammarTable.insert( i, bnfRuleVec );
        } 
    }*/
}

void removeLeftRecursion( GbnfData& data ){

}

void fullyFixGBNF( GbnfData& data ){

} 

}

