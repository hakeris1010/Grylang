#include <iostream>
#include <fstream>
#include <sstream>
#include "gbnf.h"

// TODO: This test is a stub. Complete it.
// Just sample data.

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

const char* testData2= 
  "#<a> <b>\n <a>::=\"kaka\"{{\"baka\"{{<ba>}<ca>}?<da>}*\"lala}+<woop>\n \t\xFF\tlalalalalla"
  "NOOT NOOT!!!!\\a\baifb\'''akfan{{{{}}}}}\"<tatata>};\n"
  "<zfk>:={<o>}*;<haha>:==<b>";

const char* finalData = testData2;

int main(int argc, char** argv){
    std::cout<<"Testingu-nyaa~~\n";

    gbnf::GbnfData data;
    std::istringstream strm(finalData, std::ios::in | std::ios::binary);
    
    gbnf::convertToGbnf( data, strm );

    std::cout<<"\nGBNF Result Data: \n";
    data.print(std::cout);

    return 0;
}
