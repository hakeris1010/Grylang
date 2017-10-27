/*! Program converts EBNF-style grammar file to an easily-parseable GrylloBNF format.
 *  Several options may apply: 
 *  - convert EBNF to .gbnf file
 *  - convert EBNF to C/C++ header with gBNF structures filled in with the gBNF data.
 *  - convert the .gbnf file to C/C++ header defined above.
 */ 

#include <cctype>
#include "gbnf.h"

namespace gbnf{

struct ParseInput{
    std::istream& input;
    GbnfData& data;
    int lineNum = 0;
    int posInLine = 0;
    short lastTagID = 0;

    ParseInput( std::istream& is, GbnfData& dat ) : input( is ), data( dat ) {}
};

/*! Function used to simplify the exception throwing.
 */ 
static inline void throwError(const ParseInput& inp, const char* message){
    throw std::runtime_error( "["+std::to_string(inp.lineNum)+":"+std::to_string(inp.posInLine)+"]"+ message );
}

static short getTagId( ParseInput& inp, const std::string& name, bool insertIfNotPresent ){
    // Search by iteration, because we can't search for string sorted.
    for(auto t : inp.data.tagTable){
        if(t.data == name)
            return t.ID;
    }
    // If reached this point, element not found. Insert new NonTerminal Tag if flag specified.
    if(insertIfNotPresent){
        inp.data.tagTable.insert( NonTerminal( inp.lastTagID+1, name ) );
        inp.lastTagID++;
        return inp.lastTagID;
    }
    return -1;
}

// this one is recursive
static void fillGrammarToken( ParseInput& inp, GrammarToken& tok ){

}

static void fillGrammarRule( ParseInput& inp, GrammarRule& rule ){

}

// Convert EBNF to GBNF.
void Tools::convertToGbnf(GbnfData& data, std::istream& input){
    // Automaton States represented as Enum
    const enum States { None, Comment, DefTag, DefAssignment, DefAssEquals, };

    ParseInput pinput( input );

    std::string data;
    std::string nextChars;

    States st = None;
    char c;

    while( true ){
        // Read the next character. It can be a char from buffer of chars to read next or a char from file.
        if( !nextChars.empty() ){
            c = nextChars[0];
            nextChars.erase(0, 1);
        }
        else if( !input.eof() ) // Not yet EOF'd.
            input >> c;
        }
        else{ // input EOF'd, and no chars on nextChar buffer - exit the loop depending on the state.
            // Do end jobs and break IF STATES ARE GOOD.
            break;
        }

        // We've read the next character. Now procceed with the state examination.
        switch(st){
        case None:
            if(c == '#') // Comment started
                st = Comment;
            else if(c == '<') // Definition tag-to-be-defined start.
                st = DefTag;
            else if( !std::isspace(static_cast<unsigned char>( c )) ) // If not whitespace, error.
                throwError(pinput, " : Wrong start symbol!" );
            break; // If whitespace, just stay on 'None' state.
            
        case Comment:
            if(c == '\n') // If EndLine reached, comment ends (we've only 1-line hashtag comments).
                st = None;
            break; // If any other char, still on comment.

        case DefTag:
            // Only [a-zA-Z_] are allowed on tags. Also, tag must consist of 1 or more of these chars.
            if(c == '>'){ // TagEnd
                if(data.empty())
                    throwError(pinput, " : Tag is Empty!" );
                // Actual tag has been identified. Do jobs.
                std::cout<<"Tag found! "<<data;

                st = DefAssignment // Now expect the assignment operator (::==, ::=).
                break; 
            }
            else if( !std::isalnum( static_cast<unsigned char>(c) ) && c != '_') 
                throw std::runtime_error( "["+std::to_string(lineNum)+":"+std::to_string(posInLine)+"]"+ \
                                          " : Wrong characters on tag!" );
            // Add the char read to the data string, because useful data starts now.
            data += c;

        }

        // Reset line counters if on endline.
        if(c == '\n'){
            lineNum++;
            posInLine = 0;
        }
        else
            posInLine++;


    }

}

void Tools::makeCHeaderFile(const GbnfData& data, const char* variableName, std::ostream& output){

}

}


