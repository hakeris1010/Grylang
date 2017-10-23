#include <iostream>
#include <sstream>
#include "lexer.h"

int main(int argc, char** argv){
    std::cout<<"Testing.\n";
    std::istringstream stream("Kawaii desu nee~~ :3");

    gpar::GrylangLexer lex(stream);

    std::shared_ptr<gpar::ParseNode> node;
    while( (node=lex.getNextNode()) != nullptr ){
        node->getParseData()->print( std::cout );
    /*
        if( (gpar::LexicParseData*)((node->getParseData()).get()).code == 
             gpar::GrylangLexer::LexemCode::FatalERROR ) 
  _          break;
    */ 
    }

    return 0;
}
