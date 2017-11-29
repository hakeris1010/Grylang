#include <iostream>
#include <sstream>
#include "lexer.h"

int main(int argc, char** argv){
    std::cout<<"Testing.\n";
    std::istringstream stream("string i=\"jjjj\\\"hah\";\\\" const int o = 60;", std::ios::in | std::ios::binary);
    //std::ifstream stream("../spec/program.gg", std::ios::binary);

    try{
        gpar::GrylangLexer lex(stream);

        std::shared_ptr<gpar::ParseNode> node;
        while( (node=lex.getNextNode()) != nullptr ){
            node->getParseData()->print( std::cout );
            std::cout<<"\n";
        }
    } catch(std::exception e) {
        std::cout<<"\nException caught: "<<e.what()<<"\n";
    }

    return 0;
}
