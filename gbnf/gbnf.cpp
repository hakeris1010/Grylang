/*! Program converts EBNF-style grammar file to an easily-parseable GrylloBNF format.
 *  Several options may apply: 
 *  - convert EBNF to .gbnf file
 *  - convert EBNF to C/C++ header with gBNF structures filled in with the gBNF data.
 *  - convert the .gbnf file to C/C++ header defined above.
 */ 

#include <cctype>
#include "gbnf.h"

namespace gbnf{

// Convert EBNF to GBNF.
void Tools::convertToGbnf(GbnfData& data, std::istream& input){
    // Automaton States represented as Enum
    const enum States { None, Comment, DefTagStart, DefTag, DefAssColon, DefAssEquals, };

    int lineNum = 0;
    int posInLine = 0;
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
                st = DefTagStart;
            else if( !std::isspace(static_cast<unsigned char>( c )) ) // If not whitespace, error.
                throw std::runtime_error( "["+std::to_string(lineNum)+":"+std::to_string(posInLine)+"]"+ \
                                          " : Wrong start symbol!" );
            break; // If whitespace, just stay on 'None' state.
            
        case Comment:
            if(c == '\n') // If EndLine reached, comment ends (we've only 1-line hashtag comments).
                st = None;
            break; // If any other char, still on comment.

        case DefTagStart:
            // Only [a-zA-Z_] are allowed on tags. Also, tag must consist of 1 or more of these chars.
            if( !std::isalnum( static_cast<unsigned char>(c) ) && c != '_') 
                throw std::runtime_error( "["+std::to_string(lineNum)+":"+std::to_string(posInLine)+"]"+ \
                                          " : Wrong characters on tag!" );

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


