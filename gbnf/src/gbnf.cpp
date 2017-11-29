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
#include "gbnf.h"
extern "C" {
    #include <gryltools/hlog.h>
}

namespace gbnf{

class ParseInput{
private:
    const static int SKIPMODE_NOSKIP           = 0;
    const static int SKIPMODE_NOSKIP_NOESCAPE  = 1;
    const static int SKIPMODE_SKIPWS           = 2;
    const static int SKIPMODE_SKIPWS_NONEWLINE = 3;

    std::istream& input; // Notice these are REFERENCES.
    GbnfData& data;

    int lineNum = 0;
    int posInLine = 0;
    short lastTagID = 0;

    std::string nextChars;
    std::string tempData;

    inline void throwError(const char* message) const;
    inline bool getNextString( std::string& str, size_t len=1 );
    inline bool getNextChar( char& c, int skipmode=SKIPMODE_NOSKIP );
    inline char popGetNextChar();
    inline void pushNextChar( char c );
    inline void pushNextChar( const std::string& str );
    inline void pushNextChar( const char* str, size_t size );
    inline void skipWhitespace( int skipmode );
    inline void updateLineStats( const std::string& str );
    inline void updateLineStats( char c );

    short getTagIDfromTable( const std::string& name, bool insertIfNotPresent );
    void getTagName( std::string& str );
    bool parseGrammarToken( GrammarToken& tok );
    bool parseGrammarOption( GrammarToken& tok );
    void parseGrammarRule( GrammarRule& rule );
 
public:
    ParseInput( std::istream& is, GbnfData& dat ) : input( is ), data( dat ) {}

    void convert();
};

/*! Function used to simplify the exception throwing.
 */ 
inline void ParseInput::throwError(const char* message) const {
    throw std::runtime_error( "["+std::to_string(lineNum)+":"+std::to_string(posInLine)+"]"+ message );
}

/*! Working with the priority read buffer.
 */ 
inline char ParseInput::popGetNextChar(){
    char c = 0;
    if(!nextChars.empty()){
        nextChars[ nextChars.size() - 1 ];
        nextChars.pop_back();
    }
    return c;
}

inline void ParseInput::pushNextChar( char c ){
    nextChars.push_back(c);
}

inline void ParseInput::pushNextChar( const char* str, size_t size ){
    // Ensure proper space in nextChars.
    nextChars.reserve( nextChars.size() + size );
    
    // Push the chars reversed, to ensure proper popping.
    for(const char* c = str+(size-1); c >= str; c--){
        nextChars.push_back( *c );   
    }
}

inline void ParseInput::pushNextChar( const std::string& c ){
    pushNextChar( c.c_str(), c.size() );
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

inline void ParseInput::updateLineStats( const std::string& str ) {
    size_t count = std::count( str.begin(), str.end(), '\n' );
    size_t pos = 0, lastPos = 0;
    while( (pos = str.find('\n', lastPos)) != std::string::npos ){
        this->lineNum++;
        this->posInLine = 0;
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

char ParseInput::skipWhitespace(int skipmode = SKIPMODE_SKIPWS) {
    bool ignoreNextNewLine = false;
    while( !nextChars.empty() ){
        char c = popGetNextChar();
        if(c=='\\') 
            ignoreNextNewLine = true;
        else if(c == '\n'){
            if(!ignoreNextNewLine){
                this->lineNum++;
                this->posInLine = 0;
            }
            else if(skipmode == SKIPMODE_SKIPWS_NONEWLINE)
        }
        else

        if(std::isspace( c ){
            if(skipmode==SKIPMODE_SKIPWS_NONEWLINE && c=='\n')
        }
        else{
            break;
        }
         
        this->posInLine++;
    }
    if( nextChars.empty() ){
        input >> std::ws; // Skip whaitspaces.
    }
}

/*! Gets the string of len chars from the input stream or the to-read nextChars buffer.
 *  Also updates all states, and returns false if can't read anything.
 */ 
bool ParseInput::getNextString( std::string& str, size_t len, int skipmode ){
    // Skip the whitespaces first if set so.
    if(skipmode != SKIPMODE_NOSKIP){
        if(!skipWhitespace( skipmode )) // No more characters to read.
            return false;
    }
    str.clear();
    str.reserve(len);

    // When getting chars from nextChars, we don't need to update line stats, because
    // these chars were already extracted from stream, so line counts were already updated.
    if( !nextChars.empty() ){
        // Read len chars from nextChars, and if len > nextChars.size(), read only until the end.
        size_t readStart = nextChars.size() - len;
        if(readStart < 0)
            readStart = 0;

        // Read reverse because of queue-like nature of nextChars.
        const char* endpos = nextChars.c_str() + readStart;
        for(const char* i = nextChars.c_str() + (nextChars.size()-1); i >= endpos; i--){
            str.push_back( *i );
        }
        if(str.size() >= len)
            return true;
    }
    if( !input.eof() ){ // Not yet EOF'd.
        char tmp[ len - str.size() ];
        size_t red = input.read( tmp, sizeof(tmp) );
        str.append( tmp, red );

        // Update the line stats of only the part that was extracted from stream.
        updateLineStats( str.c_str() + (str.size() - red) );
        return true;
    }
    // input EOF'd, and no chars on nextChar buffer - exit the loop depending on the state.
    return false;
}

/*! Just simplified stuff of getting the n3xtcha4r.
 */ 
inline bool ParseInput::getNextChar( char& c, int skipmode ){
    if(skipmode != SKIPMODE_NOSKIP){
        if(!skipWhitespace( skipmode )) // No more characters to read.
            return false;
    }

    // Read the next character. It can be a char from a to-read buffer or a char from file.
    if( !nextChars.empty() ){
        c = popGetNextChar();
    }
    else if( !input.eof() ){ // Not yet EOF'd.
        input >> c;
        // Update line counters when reading from a Str34m.
        updateLineStats( c );
    }
    else // input EOF'd, and no chars on nextChar buffer - return that no more to read.
        return false;
    
    return true;
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
    while( true ){
        // Preload a child token, for easier memory management
        tok.children.push_back( GrammarToken() );
        int ret = parseGrammarToken( tok.children[ tok.children.size() - 1 ] );

        // Error occured or file end reached and token did not complete.
        if(ret){ 
            tok.children.pop_back();
        }

        bool b = getNextChar(c, SKIPWS_NONEWLINE);
        if( !b || c=='<' )
            break;
        else
            throwError("Wrong option character - option start expected.");
    } 

    return true;
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
    skipWhitespace();
    getNextString(tmp, 4);

    if( !tmp.compare(0, 4, "::==") ) 
        {}
    else if( !tmp.compare(0, 3, "::=") || !tmp.compare(0, 3, ":==") )
        pushNextChar( tmp[3]);
    else if( !tmp.compare(0, 2, ":=") )
        // Insert a substring of tmp, starting at pos=2 
        pushNextChar( tmp.c_str()+2, tmp.size()-2 ); 
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
    // Automaton States represented as Enum
    enum States { None, Comment, DefTag, DefAssignment, DefAssEquals };

    States st = None;
    char c;

    while( true ){
        if( !getNextChar( c, SKIPWS ) ) // Skip whaitspeis also
            break;

        if(c == '#'){ // Comment start
            // Ignore chars until the endline, or Over 9000 of them have been read.
            input.ignore(9000, '\n'); 
            lineNum++;
        }
        else if(c == '<'){ // Rule start. Get the rule and put into the table.
            pushNextChar( c );
            
            hlogf("Getting next grammarrule\n");

            data.grammarTable.push_back( GrammarRule() );
            parseGrammarRule( data.grammarTable[ data.grammarTable.size()-1 ] );
        }
        // If non-whitespace character occured between rules and comments, error.  
        else if( !std::isspace(static_cast<unsigned char>( c )) ) 
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

