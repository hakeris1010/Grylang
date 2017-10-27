/*! Program converts EBNF-style grammar file to an easily-parseable GrylloBNF format.
 *  Several options may apply: 
 *  - convert EBNF to .gbnf file
 *  - convert EBNF to C/C++ header with gBNF structures filled in with the gBNF data.
 *  - convert the .gbnf file to C/C++ header defined above.
 */ 

#include <cctype>
#include "gbnf.h"

namespace gbnf{

class ParseInput{
private:
    std::istream& input; // Notice these are REFERENCES.
    GbnfData& data;

    int lineNum = 0;
    int posInLine = 0;
    short lastTagID = 0;

    std::string nextChars;
    std::string tempData;

    inline void throwError(const char* message) const;
    inline bool getNextString( std::string& str, size_t len=1 );

    short getTagId( const std::string& name, bool insertIfNotPresent );
    void fillGrammarToken( GrammarToken& tok );
    void fillGrammarRule( GrammarRule& rule );
 
public:
    ParseInput( std::istream& is, GbnfData& dat ) : input( is ), data( dat ) {}

    void convert();
};

/*! Function used to simplify the exception throwing.
 */ 
inline void ParseInput::throwError(const char* message){
    throw std::runtime_error( "["+std::to_string(lineNum)+":"+std::to_string(posInLine)+"]"+ message );
}

/*! Can be used to find NonTerminal tag's ID from it's name, 
 *  or to insert into a table a new NonTerminal with name = name. ID ass'd automatically.
 */ 
short ParseInput::getTagId( const std::string& name, bool insertIfNotPresent ){
    // Search by iteration, because we can't search for string sorted.
    for(auto t : data.tagTable){
        if(t.data == name)
            return t.ID;
    }
    // If reached this point, element not found. Insert new NonTerminal Tag if flag specified.
    if(insertIfNotPresent){
        data.tagTable.insert( NonTerminal( lastTagID+1, name ) );
        lastTagID++;
        return lastTagID;
    }
    return -1;
}

/*! Gets the string of len chars from the input stream or the to-read nextChars buffer.
 *  Also updates all states, and returns false if can't read anything.
 */ 
inline bool ParseInput::getNextString( std::string& str, size_t len ){
    str.reserve(len);
    if( !nextChars.empty() ){
        // Read len chars from nextChars, and if len > nextChars.size(), read only until the end.
        str.assign( nextChars, 0, len );
        if(str.size() >= len)
            return true;
    }
    if( !input.eof() ) // Not yet EOF'd.
        input.read( &(str[ str.size() ]),  ;
    }
    else{ // input EOF'd, and no chars on nextChar buffer - exit the loop depending on the state.
        // Do end jobs and break IF STATES ARE GOOD.
        break;
    }
}

inline bool ParseInput::getNextChar( char& c ){
    // Read the next character. It can be a char from a to-read buffer or a char from file.
    if( !nextChars.empty() ){
        c = nextChars[0];
        nextChars.erase(0, 1);
    }
    else if( !input.eof() ) // Not yet EOF'd.
        input >> c;
    }
    else // input EOF'd, and no chars on nextChar buffer - return that no more to read.
        return false;
    return true;
}

// this one is recursive
void ParseInput::fillGrammarToken( GrammarToken& tok ){
    
}

/*! Must start reading at the position of Tag Start ('<').
 */ 
void ParseInput::fillGrammarRule( GrammarRule& rule ){
    
}

// Convert EBNF to GBNF.
void ParseInput::convert(){
    // Automaton States represented as Enum
    const enum States { None, Comment, DefTag, DefAssignment, DefAssEquals, };

    States st = None;
    char c;

    while( true ){
        // Read the next character. It can be a char from a to-read buffer or a char from file.
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

/*! Public functions, called from outside o' this phile.
 */ 
void convertToGbnf(GbnfData& data, std::istream& input){

}

void makeCHeaderFile(const GbnfData& data, const char* variableName, std::ostream& output){

}

}


