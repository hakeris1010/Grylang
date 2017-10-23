#include "lexer.h"
#include <string>

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
    enum AutoStates {None, IdentOrKeywd, IntOrFloat, Float, CharStart, SpecChar,
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
    int automatonState = AutoStates::None;

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
        
        newNode.data.push_back(c);
        newNode.code = LexemCode::CHAR;
        break;
    }

    if(newNode.code == LexemCode::NONE)
        newNode.code = LexemCode::FatalERROR;

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

