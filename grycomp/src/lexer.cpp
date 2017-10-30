#include "lexer.h"
#include <string>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <algorithm>

namespace gpar{

GrylangLexer::GrylangLexer(std::istream& inpStream) : input( inpStream ) {}

// One-node methods
bool GrylangLexer::hasNext(){
    return this->nextNode != nullptr;
}

std::shared_ptr<ParseNode> GrylangLexer::getNextNode(){
    return getNextNode_Priv();
}

std::shared_ptr<ParseNode> GrylangLexer::getNextNode_Priv(bool getNextNode){
    // Finite automaton states, which will be used in a loop.
    enum AutoStates {None, IdentOrKeywd, IntOrFloat, Float, CharStringStart, SpecChar,
        StandardChar, StringStart, CommOrDiv, OneLineComm, MultiLineComm, MultiLineEnd, 
        UnaryOp, AssignableOp, AssignableRepeatableOp, Dash, Arrow, OperEquals, OperEqualsOperOper
    };

    std::shared_ptr<ParseNode> returnNode;
    if(this->nextNode != nullptr){
        returnNode = this->nextNode;
        this->nextNode = nullptr;
    }
    // Check if input is at error state, and nextNode == nullptr
    else if(input.eof() && nextSymbols.empty())
        return nullptr;

    // Get next node if no error is present.
    LexicParseData newNode;
    char c;
    int as = AutoStates::None;

    while(true){
        // Get first chars from the buffer of next chars read in previous loop.
        if(!nextSymbols.empty()){
            c = nextSymbols[0];
            nextSymbols.erase(0,1); // erase first element from buffer.
        }
        else if(!input.eof())
            input >> c;
        else // input.eof(), and no chars on nextSyms buffer
            break;

        // Actual automaton part
        /*
        newNode.data.push_back(c);
        newNode.code = LexemCode::CHAR;
        break;
        */

        newNode.data.push_back(c);

        // Simulate a finite automaton using a switch statement. Every state can have it's own 
        // possible outcomes.

        switch(as){
        // START.
        case None:
            if( isalpha(c) || c=='_' )
                as = IdentOrKeywd;
            else if( isdigit(c) )
                as = IntOrFloat;
            else if(c=='\'' || c=='\"') // We don't differentiate between a char and a string.
                as = CharStringStart;
            else if(c=='/')
                as = CommOrDiv;
            else if( strchr("{}[]().,:;~", c) ){
                newNode.code = LexemCode::OPERATOR;
                break;
            }
            else if( strchr("^!*%=", c) )
                as = AssignableOp;
            else if( strchr("&|+<>", c) )
                as = AssignableRepeatableOp;
            else if( c=='-' )
                as = Dash;
            else // Whitespace
                newNode.data.pop_back();
            // get out of switch, and continue iteration.
            break;

        case IdentOrKeywd:
            if( !(isalnum(c) || c=='_') ){ // No longer a word character - lexem complete.
                newNode.data.pop_back(); // Pop an extra char, and add it to next loop's first read chars
                nextSymbols.push_back(c); 
                // Check if data string (lexem) is a keyword. If not, it's an identifier.
                if(std::find(GKeywords.begin(), GKeywords.end(), newNode.data) != GKeywords.end())
                    newNode.code = LexemCode::KEYWORD;
                else
                    newNode.code = LexemCode::IDENT;
            }
            // If word character, then don't change state - just resume the loop.
            break;

        case IntOrFloat:
            if(c=='.') // Decimal point occured - it's float.
                as = Float;
            else if( !isdigit(c) ){ // Not a digit - integer end.
                newNode.code = LexemCode::INTEGER;
                newNode.data.pop_back();
                nextSymbols.push_back(c);
            }
            // If digit, don't change anything - number end is not reached.
            break; 
                 
        case Float:
            if( !isdigit(c) ){ // Not a digit - float end.
                newNode.code = LexemCode::FLOAT;
                newNode.data.pop_back();
                nextSymbols.push_back(c);
            }
            break;

        case CharStringStart:
            if( c=='\\' ) // Expect special character
                as = SpecChar;
            if( c=='\'' || c=='\"' ){
                newNode.code = LexemCode::STRING;
                newNode.data.pop_back();
            }
            // If any other, stay on this state.
            break;

        case SpecChar:
            // Skip this char (it can be anything), and just move on to next character
            as = CharStringStart;
            break;

        case CommOrDiv:
            if( c=='/' )
                as = OneLineComm;
            else if( c=='*' )
                as = MultiLineComm;
            else{
                newNode.code = LexemCode::OPERATOR;
                newNode.data.pop_back();
                nextSymbols.push_back(c);
            }
            break;

        case OneLineComm:
            if( c=='\n' ){
                newNode.code = LexemCode::COMMENT;
                newNode.data.pop_back();
            }
            // If not endline, stay on this state.
            break;

        case MultiLineComm:
            if( c=='*' )
                as = MultiLineEnd;
            // If any other, stay on this state.
            break;

        case MultiLineEnd:
            if( c=='*' )
                {} // Stay on MultiLineEnd state.
            else if(c=='/'){
                newNode.code = LexemCode::COMMENT;
                newNode.data.pop_back();
            }
            else // If any other char, move to main muliline state.
                as = MultiLineComm; 
            break;

        case AssignableOp:
            if( c=='=' )
                as = OperEquals;
            else{
                newNode.code = LexemCode::OPERATOR;
                newNode.data.pop_back();
                nextSymbols.push_back(c);
            }
            break;

        case OperEquals: ///=, &=, etc
            // No matter what char, output operator.
            newNode.code = LexemCode::OPERATOR;
            newNode.data.pop_back();
            nextSymbols.push_back(c);
            break;

        case AssignableRepeatableOp:
            if( c=='=' || c==newNode.data[0] )
                as = OperEquals;
            else{
                newNode.code = LexemCode::OPERATOR;
                newNode.data.pop_back();
                nextSymbols.push_back(c);
            }
            break;

        case Dash: // We have a '-'. Now, next char can be '-', '=', '>' (corresponding ops are -- -= ->)
            if( c=='-' || c=='=' || c=='>' )
                as = OperEquals;
            else{
                newNode.code = LexemCode::OPERATOR;
                newNode.data.pop_back();
                nextSymbols.push_back(c);
            }
            break;
        }

        // After one state switch, check if lexem found, or error occured. If yes, quit loop,
        // If no, get another symbol and continue loop with another iteration of automaton state switch.
        if(newNode.code != LexemCode::NONE)
            break;
    }

    if(newNode.code == LexemCode::NONE){
        newNode.code = LexemCode::FatalERROR;
        return nullptr;
    }

    if(getNextNode){
        this->nextNode = std::make_shared<ParseNode>( std::shared_ptr<ParseData>((ParseData*)(new LexicParseData(newNode))) );

        if(returnNode == nullptr){
            returnNode = this->nextNode;
            this->nextNode = nullptr;
            this->nextNode = this->getNextNode_Priv( false ); 
            // In this->call we set getNextNode to false, to prevent infinite recursion, 
            // because we are using this->function to get the next node.
        }
    }
    else{
        if(returnNode == nullptr)
            returnNode = std::make_shared<ParseNode>( std::shared_ptr<ParseData>((ParseData*)(new LexicParseData(newNode))) );
    }

    return returnNode;
}


}

