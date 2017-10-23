#include <iostream>
#include <sstream>
#include "lexer.h"

int main(int argc, char** argv){
    std::cout<<"Testing.\n";
    std::istringstream stream("Kawaii desu nee~~ :3");

    gpar::GrylangLexer lex(stream);

    return 0;
}
