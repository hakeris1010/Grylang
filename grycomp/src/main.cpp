#include <iostream>
#include <sstream>
#include "lexer.h"

int main(int argc, char** argv){
    std::cout<<"Testing.\n";
    std::istringstream stream("Kawaii desu nee~~ :3");

    gpar::GrylangLexer lex(stream);

    std::shared_ptr<gpar::ParseNode> node;
    while( (node=lex.getNextNode()) != nullptr ){
        std::cout<<node;

        if(node.code == gpar::GrylangLexer::LexemCode::FatalERROR) 
            break;
    }

    return 0;
}
