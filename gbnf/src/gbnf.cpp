/*! Program converts EBNF-style grammar file to an easily-parseable GrylloBNF format.
 *  Several options may apply: 
 *  - convert EBNF to .gbnf file
 *  - convert EBNF to C/C++ header with gBNF structures filled in with the gBNF data.
 *  - convert the .gbnf file to C/C++ header defined above.
 */ 

#include <cctype>
#include <iostream>
#include <algorithm>
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
    inline bool getNextChar( char& c );

    short getTagIDfromTable( const std::string& name, bool insertIfNotPresent );
    void fillGrammarToken( GrammarToken& tok );
    void fillGrammarRule( GrammarRule& rule );
 
public:
    ParseInput( std::istream& is, GbnfData& dat ) : input( is ), data( dat ) {}

    void convert();
};

/*! Function used to simplify the exception throwing.
 */ 
inline void ParseInput::throwError(const char* message) const {
    throw std::runtime_error( "["+std::to_string(lineNum)+":"+std::to_string(posInLine)+"]"+ message );
}

/*! Can be used to find NonTerminal tag's ID from it's name, 
 *  or to insert into a table a new NonTerminal with name = name. ID ass'd automatically.
 */ 
short ParseInput::getTagIDfromTable( const std::string& name, bool insertIfNotPresent ){
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

inline void ParseInput::updateLineStats( const std::string& str ) const {
    size_t count = std::count( str.begin(), str.end(), '\n' );
    size_t pos = 0, lastPos = 0;
    while( (pos = str.find('\n', lastPos)) != string::npos ){
        this->lineNum++;
        this->posInLine = 0;
        lastPos = pos;
    }
    this->posInLine += str.size()-1 - lastPos;
}

/*! Gets the string of len chars from the input stream or the to-read nextChars buffer.
 *  Also updates all states, and returns false if can't read anything.
 */ 
inline bool ParseInput::getNextString( std::string& str, size_t len ){
    str.clear();
    str.reserve(len);
    if( !nextChars.empty() ){
        // Read len chars from nextChars, and if len > nextChars.size(), read only until the end.
        str.assign( nextChars, 0, len );
        if(str.size() >= len){
            updateLineStats( str );
            return true;
        }
    }
    if( !input.eof() ){ // Not yet EOF'd.
        char tmp[ len - str.size() ];
        input.read( tmp, sizeof(tmp) );
        str.insert( str.size(), tmp, sizeof(tmp) );

        updateLineStats( str );
        return true;
    }
    // input EOF'd, and no chars on nextChar buffer - exit the loop depending on the state.
    return false;
}

/*! Just simplified stuff of getting the n3xtcha4r.
 */ 
inline bool ParseInput::getNextChar( char& c ){
    // Read the next character. It can be a char from a to-read buffer or a char from file.
    if( !nextChars.empty() ){
        c = nextChars[0];
        nextChars.erase(0, 1);
    }
    else if( !input.eof() ){ // Not yet EOF'd.
        input >> c;
    }
    else // input EOF'd, and no chars on nextChar buffer - return that no more to read.
        return false;

    // Reset line counters if on endline.
    if(c == '\n'){
        this->lineNum++;
        this->posInLine = 0;
    }
    else
        this->posInLine++;

    return true;
}

/*! Gets the name of the tag at current position. Can start with a '<' or a tag-compatibru letter.
 */ 
void getTagName( std::string& str ){
    char c = 0;
    if(getNextChar(c)){
        if(c != '<') // Ignore the '<'. If char is not a '<', add it to nexttoreads.
            nextChars.insert(0, 1, c);
    }

    while( getNextChar(c) ){
        // Tag chars: [a-zA-Z_]
        if( !std::isalnum( static_cast<unsigned char>(c) ) && c != '_' ) 
            throwError( "Wrong character in a tag!" );
        str += c;
    }
}

// this one is recursive
void ParseInput::fillGrammarToken( GrammarToken& tok ){
    
}

/*! Must start reading at the position of Tag Start ('<').
 */ 
void ParseInput::fillGrammarRule( GrammarRule& rule ){
    std::string tmp;
    tmp.reserve(256);
    
    getTagName( tmp );

    rule.ID = getTagIDfromTable( tmp, true ); // Add to table if not present.
}

// Convert EBNF to GBNF.
void ParseInput::convert(){
    // Automaton States represented as Enum
    enum States { None, Comment, DefTag, DefAssignment, DefAssEquals };

    States st = None;
    char c;

    while( true ){
        if( !getNextChar( c ) )
            break;

        if(c == '#'){ // Comment start
            input.ignore(9000, '\n'); // Ignore chars until the endline, or Over 9000 of them are read.
            lineNum++;
        }
        else if(c == '<'){ // Rule start. Get the rule and put into the table.
            nextChars += c;
            data.grammarTable.push_back( GrammarRule() );
            fillGrammarRule( data.grammarTable[ data.grammarTable.size()-1 ] );
        }
        else if( !std::isspace(static_cast<unsigned char>( c )) ) // If not whitespace, error.
            throwError(" : Wrong start symbol!" );
    }
        // If whitespace, just continue on 'None' state.

       /*case DefTag:
        // Only [a-zA-Z_] are allowed on tags. Also, tag must consist of 1 or more of these chars.
        if(c == '>'){ // TagEnd
            if(tempData.empty())
                throwError(" : Tag is Empty!" );
            // Actual tag has been identified. Do jobs.
            std::cout<<"Tag found! "<<tempData;

            st = DefAssignment; // Now expect the assignment operator (::==, ::=).
            break; 
        }
        else if( !std::isalnum( static_cast<unsigned char>(c) ) && c != '_') 
            throw std::runtime_error( "["+std::to_string(lineNum)+":"+std::to_string(posInLine)+"]"+ \
                                      " : Wrong characters on tag!" );
        // Add the char read to the data string, because useful data starts now.
        tempData += c;
        */
}

/*! Public functions, called from outside o' this phile.
 */ 
void convertToGbnf(GbnfData& data, std::istream& input){

}

void makeCHeaderFile(const GbnfData& data, const char* variableName, std::ostream& output){

}

}


