/*! Program converts EBNF-style grammar file to an easily-parseable GrylloBNF format.
 *  Several options may apply: 
 *  - convert EBNF to .gbnf file
 *  - convert EBNF to C/C++ header with gBNF structures filled in with the gBNF data.
 *  - convert the .gbnf file to C/C++ header defined above.
 */ 

#include <string>
#include <iostream>
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cctype>
#include <gryltools/stackreader.hpp>
#include "gbnf.h"
extern "C" {
    #include <gryltools/hlog.h>
}

namespace gbnf{

struct ParseState{
    size_t line = 0;
    size_t pos = 0;
    short lastTagID = 0;
}

class ParseInput{
private:
    gtools::StackReader reader;
    GbnfData& data;

    ParseState ps;
    std::string tempData;

    inline void throwError(const char* message) const;
    inline void updateLineStats( const std::string& str );
    inline void updateLineStats( char c );

    short getTagIDfromTable( const std::string& name, bool insertIfNotPresent );
    void getTagName( std::string& str );
    bool parseGrammarToken( GrammarToken& tok );
    bool parseGrammarOption( GrammarToken& tok );
    void parseGrammarRule( GrammarRule& rule );
 
public:
    ParseInput( std::istream& is, GbnfData& dat ) : reader( is ), data( dat ) {}

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
        if(t.data.compare( name ) == 0)
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

inline void ParseInput::updateLineStats( const std::string& str ) {
    size_t pos = 0, lastPos = 0;
    while( (pos = str.find('\n', lastPos)) != std::string::npos ){
        this->lineNum++;
        lastPos = pos;
    }
    this->posInLine += str.size()-1 - lastPos;
}

inline void ParseInput::updateLineStats( char c ) {
    if(c == '\n'){
        this->lineNum++;
        this->posInLine = 0;
    }
    else
        this->posInLine++;
}

/*! Gets the name of the tag at current position. Can start with a '<' or a tag-compatibru letter.
 */ 
void ParseInput::getTagName( std::string& str ){
    char c = 0;
    if(getNextChar(c)){
        if(c != '<') // Ignore the '<'. If char is not a '<', add it to nexttoreads.
            pushNextChar( c);
    }

    while( getNextChar(c) ){
        if( c == '>' ){ // End
            if( str.empty() )
                throwError( "Tag is empty!" );
            break; // if string is not empty, break and return good.    
        }
        // Tag chars: [a-zA-Z_]
        else if( !std::isalnum( static_cast<unsigned char>(c) ) && c != '_' ){
            throwError( "Wrong character in a tag!" );
        }
        str += c;
    }
}

/*! Gets next token
 */ 
bool ParseInput::parseGrammarToken( GrammarToken& tok ){

    return true;
}

/*! Get a Grammar Option (Root Token).
 *  - Consists of several child tokens, of which some can be nested or even recursive.
 *  @param tok - a reference to a ready GrammarToken structure.
 */ 
bool ParseInput::parseGrammarOption( GrammarToken& tok ){
    tok.type = GrammarToken::ROOT_TOKEN;
    /*tok.id = 1337;
    tok.size = 0;
    tok.data = "Nyaa~";
    */
    char c;
    while( reader.getChar( c, gtools::StackReader::SKIPMODE_SKIPWS ) ){
        // Check if option end (a pipe symbol)
        if( c == '|' )
            break;

        // NO!!!! Check if c is a line-concat symbol ( backslash ).
        if( c == '\\' ){
            skipmode = gtools::StackReader::SKIPMODE_SKIPWS;
            continue;
        }
        



        // Preload a child token, for easier memory management
        tok.children.push_back( GrammarToken() );
        int ret = parseGrammarToken( tok.children[ tok.children.size() - 1 ] );

        // Error occured or file end reached and token did not complete.
        if(ret){ 
            tok.children.pop_back();
        }

        if( !b || c=='<' )
            break;
        else
            throwError("Wrong option character - option start expected.");

        skipmode = gtools::StackReader::SKIPMODE_SKIPWS_NONEWLINE;
    } 
    
    return tok.children.empty();
}

/*! Parse the grammar rule definition. 
 *  Must start reading at the position of Tag Start ('<').
 *  @param rule - the rule structure to parse data into.
 */ 
void ParseInput::parseGrammarRule( GrammarRule& rule ){
    std::string tmp;
    tmp.reserve(256);

    hlogf("Getting TagName\n");
    
    // Get the first tag (the NonTerminal this rule defines), and it's ID.
    getTagName( tmp );
    rule.ID = getTagIDfromTable( tmp, true ); // Add to table if not present.

    // Get the definition-assignment operator (::==, ::=, :==, :=).
    reader.getString( &tmp[0], 4, gtools::StackReader::SKIPMODE_SKIPWS, ps.line, ps.pos );

    if( !tmp.compare(0, 4, "::==") ) 
        {}
    else if( !tmp.compare(0, 3, "::=") || !tmp.compare(0, 3, ":==") )
        reader.pushChar( tmp[3] );
    else if( !tmp.compare(0, 2, ":=") )
        reader.pushString( tmp.c_str()+2, 2 ); 
    else
        throwError("No Def-Assignment operator on a rule");

    // Get options (ROOT type tokens), one by one, in a loop
    GrammarToken tok;
    while( parseGrammarOption( tok ) ){
        rule.options.push_back(tok);

        hlogf("Got Token: %d, %s\n", tok.id, tok.data.c_str());
    }
    // We've parsed a rule. All options are parsed.
}

// Convert EBNF to GBNF.
void ParseInput::convert(){
    char c;
    while( true ){
        // Get first non-whitespace character.
        if( !reader.getChar( c, gtools::StackReader::SKIPMODE_SKIPWS, ps.line, ps.pos ) ) 
            break;

        // Check possibilities: comment, rule.
        if(c == '#'){ // Comment start
            // Ignore chars until the endline, or Over 9000 of them have been read.
            if( !reader.skipUntilChar('\n') )
                break; 
            ps.line++;
        }
        else if(c == '<'){ // Rule start. Get the rule and put into the table.
            reader.putChar( c );
            
            hlogf("Getting next grammarrule\n");

            data.grammarTable.push_back( GrammarRule() );
            parseGrammarRule( data.grammarTable[ data.grammarTable.size()-1 ] );
        }
        else // If other non-whitespace character occured between rules and comments, error.  
            throwError(" : Wrong start symbol!" );
    }
}

/*! Public functions, called from outside o' this phile.
 */ 
void convertToGbnf(GbnfData& data, std::istream& input){
    //hlogSetFile("grylogz.log", HLOG_MODE_APPEND);
    hlogSetActive(true);

    ParseInput pi( input, data );
    pi.convert();
}

void makeCHeaderFile(const GbnfData& data, const char* variableName, std::ostream& output){

}

/*! Prints the GBNF Data to stream passed.
 */
void GbnfData::print( std::ostream& os ){
    os<<"GBNFData in 0x"<<this<<"\n Flags:"<<flags<<"\n TagTable:\n ";
    for(auto a : tagTable)
        os<<" ["<<a.ID<<"]: "<<a.data<<"\n";
    os<<"\nGrammarTable: "<<grammarTable.size()<<" entries.\n";
}

}

/*
 * Set, Close and Get the current LogFile
 FILE* hlogSetFile(const char* fname, char mode);
 void hlogSetFileFromFile(FILE* file, char mode);
 void hlogCloseFile();
 FILE* hlogGetFile();
 char hlogIsActive();
 void hlogSetActive(char val);
 
 * Write to the LogFile (Printf style) 
 void hlogf( const char* fmt, ... );
*/

