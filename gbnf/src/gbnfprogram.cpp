#include <iostream>
#include <fstream>
#include <sstream>
#include "gbnf.hpp"

const char* testData = 
  "<trans_unit> ::== {<ext_object>}* ;              \n" 
  "<ext_object> ::== <declaration>                  \n" 
  "                 | <ext_variable_definition>     \n" 
  "                 | <function_definition>         \n" 
  "                 | <class_definition> ;          \n" 
  "                                                 \n" 
  "<variable_declaration> ::== <typespec> <ident> ; \n" 
  "                                                 \n" 
  "<function_declaration> ::== \"fun\" <ident>      \n" 
  "              <param_list> {\":\" <typespec> } ? \n" 
  "             | <fundecc> ;                       \n" 
  "                                                 \n"   
  "<class_inheritance> ::== <extend_specifier>      \n" 
  "                  <ident> {  \",\" <ident> } *;  \n" 
  "                                                 \n" 
  "<extend_specifier> ::== \"extends\"              \n" 
  "                      | \"implements\"           \n" 
  "<eee>                                            \n";

const char* finalData = testData;

int main(int argc, char** argv){
    // Properties
    /*std::ofstream outFile;
    bool verbose;

    // Parse arguments.
    if(argc > 1){
        
    }*/

    gbnf::GbnfData data;
    std::istringstream strm(finalData, std::ios::in | std::ios::binary);
    
    gbnf::convertToGbnf( data, strm );

    //std::cout<<"\nGBNF Result Data: \n";
    //data.print(std::cout);

    //std::cout<<"\n==============================\nGenerating C++ code...\n";
    gbnf::generateCode( data, std::cout, "parsingData");

    return 0;
}

