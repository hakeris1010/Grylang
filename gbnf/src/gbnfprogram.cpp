#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <functional>
#include <cstring>
#include <map>
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

/*"<keyword> ::== \"char\" | \"int\" | \"int16\" | \"int32\" | \"int64\" | \"float\" | \"double\" | \"void\" | \"var\"\n"
"             | \"fun\" | \"class\" | \"private\" | \"protected\" | \"public\" | \"extends\" | \"implements\" \n"
"             | \"const\" | \"volatile\" | \"if\" | \"else\" | \"switch\" | \"case\" | \"default\" | \"while\" \n"
"             | \"for\" | \"foreach\" | \"in\" | \"break\" | \"goto\" | \"return\" ;\n"
"\n"
"<ident> ::== \"[a-zA-Z_]\"{<w>}* ;\n"
"\n"
"<integer_constant> ::== {<d>}+ ;\n"
"\n" 
*/

const char* testData2 = 
//"<character_constant> ::== \"\\'\" {<p>}* \"\\'\" ;\n" 
"<noot> ::= \"woop[]\" {<baka> <desu> \"abcd\" { \"regex\" \"+\" }* }+ <noot> ;\n"
;

const char* finalData = testData2;

static bool debug = false;

struct BnfInputFile{
    std::shared_ptr< std::istream > is;
    std::string filename;
    std::string type;

    BnfInputFile( std::shared_ptr< std::istream > isptr, 
                  const std::string& fname, const std::string& _type ="" )
        : is( isptr ), filename( fname ), type( _type )
    {}

    BnfInputFile( const std::string& fname, 
                  std::ios_base::openmode mode = std::ios_base::in | std::ios_base::binary, 
                  const std::string& _type = "" )
        : is( new std::ifstream( fname, mode ) ), filename( fname ), type( _type )
    {}

    static bool compare( const BnfInputFile& a, const BnfInputFile& b ){
        return a.filename < b.filename;
    }
};

int main(int argc, char** argv){
    // Properties
    std::set< BnfInputFile, bool(*)(const BnfInputFile&, const BnfInputFile&) > 
        inFiles ( BnfInputFile::compare );

    std::ofstream outFile;
    std::string outFileName;

    int verbosity = 0;
    bool convertToBnf = false;
    int recursionFixMode = 0;

    // Parse arguments.
    if(argc > 1){
        for(int i=1; i<argc; i++){
            // Check for flag switches
            if( (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) )
                verbosity++;
            else if(!strcmp(argv[i], "-vv") || !strcmp(argv[i], "--mega-verbose")) 
                verbosity += 2;
            else if(!strcmp(argv[i], "-vvv") || !strcmp(argv[i], "--ultra-verbose")) 
                verbosity += 3; 

            else if( !strcmp(argv[i], "--debug=true") || !strcmp(argv[i], "--debug") )
                debug = true;
            else if( !strcmp(argv[i], "--debug=false") || !strcmp(argv[i], "--nodebug") )
                debug = false;

            else if(!strcmp(argv[i], "--convert-to-bnf=true") || 
                    !strcmp(argv[i], "--convert-to-bnf"))
                convertToBnf = true;
            else if(!strcmp(argv[i], "--convert-to-bnf=false"))
                convertToBnf = false; 

            else if(!strcmp(argv[i], "--fix-recursion=left"))
                recursionFixMode = gbnf::FIX_LEFT_RECURSION;
            else if(!strcmp(argv[i], "--fix-recursion=right"))
                recursionFixMode = gbnf::FIX_RIGHT_RECURSION;

            // Output file is indicated by "-o"
            else if((!strcmp(argv[i], "-o") || !strcmp(argv[i], "--outfile")) && i < argc-1){
                i++;
                outFile.open( argv[i], std::ios::out | std::ios::binary );

                if(!outFile.is_open())
                    std::cerr<<"Can't open output file \""<< argv[i] <<"\"!\n";
                else
                    outFileName = std::string( argv[i] );
            }
            else{ // If any other argument, we will treat it as an input file.
                auto ff = std::make_shared< std::ifstream >( 
                    argv[i], std::ios::in | std::ios::binary
                ); 

                if( ff->is_open() )
                    inFiles.insert( BnfInputFile( ff, argv[i] ) );
                else
                    std::cerr<<"Can't open input file \""<< argv[i] <<"\"!\n";
            }
        }
    }

    // Set final output - cout if no set.
    std::ostream& output = ( outFile.is_open() ? outFile : std::cout );

    // Fix input if no found - cin if no present.
    if( inFiles.empty() ){
        if( !debug ){
            // When making pointer to cin, set deallocator to empty function.
            inFiles.insert( BnfInputFile(
                 std::shared_ptr<std::istream>( &std::cin, [](void* p){} ),
                 "std_standard_input"
            ) );
        } 
        else{ // In debug mode . . . 
            inFiles.insert( BnfInputFile(
                std::make_shared<std::istringstream>( finalData, 
                    std::ios::in | std::ios::binary ),
                "test_stringStreamData"
            ) );
        }
    }

    // Setup output file name if we haven't done already.
    if( outFileName.empty() ){
        outFileName = (inFiles.begin())->filename;
    }

    // Output everything if mega verbose.
    if( verbosity > 1 ){
        std::cout<<"Final setup:\n inFiles: "<< inFiles.size() <<"\n debug: "<<debug;
        std::cout<<"\n verbosity: "<<verbosity<<"\n convertToBnf: "<<convertToBnf;
        std::cout<<"\n recursionFixMode: "<< recursionFixMode <<"\n\n";
    }

    gbnf::CodeGenerator gen( output, outFileName );
    gen.outputStart();

    // Run through each input, and produce an output
    for( auto& a : inFiles ){
        if( verbosity > 0)
            std::cout<<"\nParsing file: "<< a.filename <<"\n";

        // Allocate data.
        gbnf::GbnfData data;

        // Parse file to GBNF
        gbnf::convertToGbnf( data, *(a.is), verbosity-1 );

        if( verbosity > 0)
            std::cout<<" Parsed to GBNF. No. of Rules: "<< data.grammarTable.size() <<"\n";

        // Convert to BNF
        if( convertToBnf ){
            gbnf::convertToBNF( data, ( recursionFixMode == 
                gbnf::FIX_LEFT_RECURSION ? true : false ), verbosity-1 );

            if( verbosity > 0){
                std::cout<<" Converted to BNF. No. of Rules: "<< 
                    data.grammarTable.size() <<"\n";
            }
        }

        // Fixing recursion
        if( recursionFixMode ){    
            if( verbosity > 0)
                std::cout<<" Fixing recursion: ";
            std::cout<< (recursionFixMode==gbnf::FIX_LEFT_RECURSION ? "left" : "right") <<"\n";
        
            gbnf::fixRecursion( data, recursionFixMode, verbosity-1 );
        }
    
        // Generating
        if( verbosity > 0)
            std::cout<<" Generating Code ... \n";

        gen.generateConstructionCode( data, a.filename, verbosity-1 ); 
    }

    gen.outputEnd();

    return 0;
}

