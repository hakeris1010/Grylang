#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <cstring>
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
    std::vector< std::unique_ptr<std::istream> > inFiles;
    std::ofstream outFile;
    bool verbose = false;
    bool megaVerbose = false;
    bool convertToBnf = false;
    bool fixRecursion = true;

    // Parse arguments.
    if(argc > 1){
        for(int i=1; i<argc; i++){
            // Check for flag switches
            if((!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) && !verbose)
                verbose = true;
            else if(((!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) && verbose) ||
                    (!strcmp(argv[i], "-vv") || !strcmp(argv[i], "--mega-verbose")) ){
                verbose = true;
                megaVerbose = true;
            }

            else if(!strcmp(argv[i], "--convert-to-bnf=true") || 
                    !strcmp(argv[i], "--convert-to-bnf"))
                convertToBnf = true;
            else if(!strcmp(argv[i], "--convert-to-bnf=false"))
                convertToBnf = false; 

            else if(!strcmp(argv[i], "--fix-recursion=true") ||
                    !strcmp(argv[i], "--fix-recursion"))
                fixRecursion = true;
            else if(!strcmp(argv[i], "--fix-recursion=false"))
                fixRecursion = false;

            // Output file is indicated by "-o"
            else if((!strcmp(argv[i], "-o") || !strcmp(argv[i], "--outfile")) && i < argc-1){
                i++;
                outFile.open( argv[i], std::ios::out | std::ios::binary );
                if(!outFile.is_open())
                    std::cerr<"Can't open output file \""<< argv[i] <<"\"!\n";
            }
            else{ // If any other argument, we will treat it as an input file.
                inFiles.push_back( std::make_unique< std::istream >(
                    std::ifstream( argv[i], std::ios::in | std::ios::binary ) 
                ) );
                if( ! *(inFiles[ inFiles.size() - 1 ].get()).is_open() ){
                    std::cerr<<"Can't open input file \""<< argv[i] <<"\"!\n";
                    inFiles.pop_back();
                }
            }
        }
    }

    // Set final output - cout if no set.
    std::ostream& output = ( outFile.is_open() ? outFile : std::cout );

    // Fix input if no found - cin if no present.
    if( inFiles.empty() ){
        // When getting pointer to cin, set deallocator to empty function.
        inFiles.push_back( 
            std::make_unique< std::istream >( &std::cin, []( void* p ){} );
        );
    }

    // Output everything if mega verbose.
    if( megaVerbose ){
        std::cout<<"Final setup:\n inFiles: "<< inFiles.size() <<"\n";
        std::cout<<" verbose: "<<verbose<<"\n megaVerbose: "<<megaVerbose<<"\n";
        std::cout<<" convertToBnf: "<<convertToBnf<<", fixRecursion: "<<fixRecursion<<"\n\n";
    }

    gbnf::GbnfData data;
    std::istringstream strm(finalData, std::ios::in | std::ios::binary);
    
    gbnf::convertToGbnf( data, strm );

    //std::cout<<"\nGBNF Result Data: \n";
    //data.print(std::cout);

    //std::cout<<"\n==============================\nGenerating C++ code...\n";
    gbnf::generateCode( data, std::cout, "parsingData");

    return 0;
}

