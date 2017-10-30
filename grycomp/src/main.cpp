#include <iostream>
#include <sstream>
#include "lexer.h"

int main(int argc, char** argv){
    std::cout<<"Testing.\n";
    //std::istringstream stream("int i=50; const int o = 60;", std::ios::in | std::ios::binary);
    std::ifstream stream("../spec/program.gg", std::ios::binary);

    gpar::GrylangLexer lex(stream);

    std::shared_ptr<gpar::ParseNode> node;
    while( (node=lex.getNextNode()) != nullptr ){
        node->getParseData()->print( std::cout );
        std::cout<<"\n";
    /*
        if( (gpar::LexicParseData*)((node->getParseData()).get()).code == 
             gpar::GrylangLexer::LexemCode::FatalERROR ) 
  _          break;
    */ 
    }

    return 0;
}
