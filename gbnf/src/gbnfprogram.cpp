#include <iostream>
#include <fstream>
#include <sstream>
#include "gbnf.h"

const char* testData = \
  "<trans_unit> ::== {<ext_object>}*           \n" \
  "<ext_object> ::== <declaration>             \n" \
  "                 | <ext_variable_definition>\n" \
  "                 | <function_definition>    \n" \
  "                 | <class_definition>       \n";

int main(int argc, char** argv){
    std::cout<<"Testingu-nyaa~~\n";

    gbnf::GbnfData data;
    std::istringstream strm(testData, std::ios::in | std::ios::binary);
    
    gbnf::convertToGbnf( data, strm );
    data.print(std::cout);

    return 0;
}

