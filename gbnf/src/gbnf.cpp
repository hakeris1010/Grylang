/*! Program converts EBNF-style grammar file to an easily-parseable GrylloBNF format.
 *  Several options may apply: 
 *  - convert EBNF to .gbnf file
 *  - convert EBNF to C/C++ header with gBNF structures filled in with the gBNF data.
 *  - convert the .gbnf file to C/C++ header defined above.
 */ 

#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
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
};

class ParseInput{
private:
    gtools::StackReader reader;
    GbnfData& data;

    ParseState ps;
    std::string tempData;

    template<typename... Args>
    inline void logf( bool debug, const char* message, Args&&... args ) const;

    inline void throwError( const std::string& message ) const; 
    inline void updateLineStats( const std::string& str );
    inline void updateLineStats( char c );

    short getTagIDfromTable( const std::string& name, bool insertIfNotPresent );
    void getTagName( std::string& str, bool startsWithLetter = true );
    int  parseGrammarToken( GrammarToken& tok, int recLevel = 1, char endChar = '}' );
    bool parseGrammarOption( GrammarToken& tok );
    void parseGrammarRule( GrammarRule& rule );
 
public:
    ParseInput( std::istream& is, GbnfData& dat ) : reader( is ), data( dat ) {}

    void convert();
};

/*! Function used to simplify the exception throwing.
 */ 
inline void ParseInput::throwError( const std::string& message ) const {
    std::stringstream ss;
    ss <<"["<< std::to_string( ps.line ) <<":"<< std::to_string( ps.pos ) <<"] "<< message;
    throw std::runtime_error( ss.str() );
}

/*! Simplified logging mechanism.
 */ 
template<typename... Args>
inline void ParseInput::logf( bool dewit, const char* message, Args&&... args ) const {
    if(!dewit)
        return;
    hlogf( message, std::forward<Args>( args )... );
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
        data.tagTable.insert( NonTerminal( ps.lastTagID+1, name ) );
        ps.lastTagID++;
        return ps.lastTagID;
    }
    return -1;
}

inline void ParseInput::updateLineStats( const std::string& str ) {
    size_t pos = 0, lastPos = 0;
    while( (pos = str.find('\n', lastPos)) != std::string::npos ){
        ps.line++;
        lastPos = pos;
    }
    ps.pos += str.size()-1 - lastPos;
}

inline void ParseInput::updateLineStats( char c ) {
    if(c == '\n'){
        ps.line++;
        ps.pos = 0;
    }
    else
        ps.pos++;
}

/*! Gets the name of the tag at current position. 
 *  @param str - a buffer to which to write a tag.
 *  @param startsWithLetter - if false, tag starts with a '<',
 *         if true - with a tag-compatibru letter.
 */ 
void ParseInput::getTagName( std::string& str, bool startsWithLetter ){
    char c = 0;

    if( reader.peekChar() == '<' ) // Starts with a '<' - skip it.
        reader.getChar( c );

    while( reader.getChar( c ) ){
        updateLineStats( c );

        if( c == '>' ){ // End
            if( str.empty() )
                throwError( "Tag is empty!" );
            break; // if string is not empty, break and return good.    
        }
        // Tag chars: [a-zA-Z_]
        else if( !std::isalnum( static_cast<unsigned char>(c) ) && c != '_' ){
            throwError( "Wrong character in a tag!" );
        }
        str.push_back( c );
    }
    
    if( c != '>' )
        throwError( "Tag hasn't ended!" );
}

/*! Gets next grammar Token. It's recursive.
 *  - When called, the Reader position must be at first token's character.
 */ 
int ParseInput::parseGrammarToken( GrammarToken& tok, int recLevel, char endChar ){
    const bool dbg = false;

    const static int END_OF_STREAM             = 2;
    const static int RECURSIVE_ENDCHAR_REACHED = 1;
    //const static int NORMAL_SUCCESS            = 0;

    std::string recs( recLevel, ' ' );
    logf(dbg,"%s[parseGrammarToken(_,_,\'%c\']\n", recs.c_str(), endChar);

    // Check if token start character.
    char c;
    std::string buff;

    // Get the next character, skipping any whitespaces.
    if( !reader.getChar( c, gtools::StackReader::SKIPMODE_SKIPWS, ps.line, ps.pos ) )
        return END_OF_STREAM;

    // Check all valid token start characters.
    // Non-Terminal 
    if( c == '<' ){
        logf(dbg,"%sTag recognized... \n", recs.c_str());

        buff.reserve(32); // Reserve at least N chars for the upcoming tagname.
        getTagName( buff, true );

        logf(dbg,"%sGot Name:%s\n", recs.c_str(), buff.c_str());

        tok.type = GrammarToken::TAG_ID;
        tok.id = getTagIDfromTable( buff, true );
    }
    // Regex-String
    else if( c == '\"' ){
        logf(dbg,"%sString recognized... \n", recs.c_str());

        tok.type = GrammarToken::REGEX_STRING;
        register bool afterEscape = false;

        while( reader.getChar( c ) ){
            updateLineStats( c );

            // Handle escapes
            if( c == '\\' )
                afterEscape = true;
            else if( c=='\"' && !afterEscape )
                break;

            // If any character, add it to our string.
            tok.data.push_back( c );

            if(afterEscape)
                afterEscape = false;
        }
        if( c != '\"' ) // Wrong end
            throwError( "String hasn't ended!" );

        logf(dbg,"%sData: \"%s\"\n", recs.c_str(), tok.data.c_str());
    }
    // Group. Several repeat types included.
    else if( c == '{' ){
        // Allocate a token.
        GrammarToken temp;

        logf(dbg,"%sRecursive Group start recognized. Getting childs...\n", recs.c_str());

        // Start recursive iteration through tokens.
        // Loop while the return value is 0, i.e. full valid token has been extracted.
        // If return value is 1, end character has been reached (the '}' character).
        while( !parseGrammarToken( temp, recLevel+1, '}' ) ){
            tok.children.push_back( temp );
            temp = GrammarToken(); // Allocate new token.
        }

        // Group ended. Now get the group's repeat-type character.
        if( !reader.getChar( c, gtools::StackReader::SKIPMODE_SKIPWS, ps.line, ps.pos ) )
            c = '1'; // Just a group.

        if( c == GrammarToken::GROUP_OPTIONAL    || 
            c == GrammarToken::GROUP_REPEAT_NONE ||
            c == GrammarToken::GROUP_REPEAT_ONE )
            tok.type = c;
        // If still group, but next char is not a repeat-type.
        else{
            tok.type = GrammarToken::GROUP_ONE;

            // Putback the char for next readings.
            reader.putChar( c ); 
        }

        logf(dbg, "%sGroup ended. Group type: [ %c ], Child Count: %d\n", 
               recs.c_str(), tok.type, tok.children.size() );
    }
    // Check if current character is a recursive group end character. 
    else if( c == endChar ){
        logf(dbg,"%sRecursive Group ended. End char: \'%c\'\n\n", recs.c_str(), c );

        return RECURSIVE_ENDCHAR_REACHED;  // Returned from a recursion.
    }
    // Other character - just Throw an Error and be happy.
    else 
        throwError( "Wrong token start symbol: "+std::string(1, c) );

    //logf(dbg,"%sSuccessfully extracted a token!\n", recs.c_str());
    logf(dbg,"\n");
    return 0; //NORMAL_SUCCESS; // Success.
}

/*! Get a Grammar Option (Root Token).
 *  - Consists of several child tokens, of which some can be nested or even recursive.
 *  @param tok - a reference to a ready GrammarToken structure.
 *  @return - true if expecting more options, false if otherwise (rule ended or stream ended)
 */ 
bool ParseInput::parseGrammarOption( GrammarToken& tok ){
    const bool dbg = false;

    // Option is a ROOT Token, assign this type.
    tok.type = GrammarToken::ROOT_TOKEN;
    char c;

    logf(dbg,"[parseGrammarOption(_)]\n");

    while( reader.getChar( c, gtools::StackReader::SKIPMODE_SKIPWS, ps.line, ps.pos ) ){
        // Check if option end (a pipe symbol) - just return true, and expect next option.
        if( c == '|' )
            return true;

        // Whole rule end - return false, don't expect more options. 
        else if( c == ';' )
            return false;

        // Other character means that token start occured. Put it back to stream.
        reader.putChar( c );

        // Preload a child token, for easier memory management
        tok.children.push_back( GrammarToken() );
        int ret = parseGrammarToken( tok.children[ tok.children.size() - 1 ] );

        // Non-Fatal error occured, recursive group ended,
        // or file end reached and token did not complete.
        // Just pop out the token, and that's it.
        if(ret) 
            tok.children.pop_back();
    } 
    // If stream has ended, return false - no more chars can be read.
    return false;
}

/*! Parse the grammar rule definition. 
 *  Must start reading at the position of Tag Start ('<').
 *  @param rule - the rule structure to parse data into.
 */ 
void ParseInput::parseGrammarRule( GrammarRule& rule ){
    const bool dbg = false;

    std::string tmp;
    tmp.reserve(256);

    logf(dbg,"[parseGrammarRule(_)]\nGetting TagName... \n");
    
    // Get the first tag (the NonTerminal this rule defines), and it's ID.
    getTagName( tmp );
    rule.ID = getTagIDfromTable( tmp, true ); // Add to table if not present.

    logf(dbg,"Name: %s, ID: %d \nGetting assignment OP...", tmp.c_str(), (int)rule.ID);

    // Get the definition-assignment operator (::==, ::=, :==, :=).
    reader.getString( &tmp[0], 4, gtools::StackReader::SKIPMODE_SKIPWS, ps.line, ps.pos );

    if( !strncmp(tmp.c_str(), "::==", 4) ) 
        {}
    else if( !strncmp(tmp.c_str(), "::=", 3) || !strncmp(tmp.c_str(), ":==", 3) )
        reader.putChar( tmp[3] );
    else if( !strncmp(tmp.c_str(), ":=", 2) )
        reader.putString( tmp.c_str()+2, 2 );  
    else
        throwError("No Def-Assignment operator on a rule");

    // Get options (ROOT type tokens), one by one, in a loop
    bool areMore = true;

    logf(dbg,"Getting Options in a Loop...\n\n");

    while( areMore ){
        rule.options.push_back( GrammarToken() );
        GrammarToken& tok = rule.options[ rule.options.size()-1 ];

        areMore = parseGrammarOption( tok ); 

        // Accept only non-empty option tokens.
        if( tok.children.empty() )
            rule.options.pop_back();

        logf(dbg,"Got Option: Count of Childs: %d\n\n", tok.children.size());
    }

    // We've parsed a rule. All options are parsed.
    logf(dbg,"Rule finished!\n============================\n\n");
}

// Convert EBNF to GBNF.
void ParseInput::convert(){
    const bool dbg = false;

    char c;
    while( true ){
        // Get first non-whitespace character.
        if( !reader.getChar( c, gtools::StackReader::SKIPMODE_SKIPWS, ps.line, ps.pos ) ) 
            break;

        // Check possibilities: comment, rule.
        if(c == '#'){ // Comment start
            logf(dbg, "Comment started. Skipping until \\n...");

            // Ignore chars until the endline, or Over 9000 of them have been read.
            if( !reader.skipUntilChar('\n') )
                break; 
            //ps.line++;
        }
        else if(c == '<'){ // Rule start. Get the rule and put into the table.
            reader.putChar( c );
            
            logf(dbg, "Grammar Rule started. Getting it...\n");

            data.grammarTable.push_back( GrammarRule() );
            parseGrammarRule( data.grammarTable[ data.grammarTable.size()-1 ] );
        }
        else // If other non-whitespace character occured between rules and comments, error.  
            throwError(" : Wrong start symbol!" );
    }
}

/*! GBNF Structure Printers. 
 *  Prints the GBNF Data to stream passed.
 */
void GbnfData::print( std::ostream& os, int mode, const std::string& ld ) const {
    os << ld <<"GBNFData in 0x"<<this<<"\n"<<ld<<" Flags:"<<flags<<"\n"
       << ld <<" TagTable ("<< tagTable.size() <<" entries):\n";
    for(auto a : tagTable)
        os<<ld<<" [ "<<a.ID<<" ]: "<<a.data<<"\n";

    os<<"\n"<<ld<<"GrammarTable: ("<<grammarTable.size()<<" entries):\n";
    for(auto a : grammarTable)
        a.print( os, mode, ld + " " );
}

void GrammarRule::print( std::ostream& os, int mode, const std::string& ld ) const {
    os <<"\n"<< ld << "[GrammarRule]: Defining NonTerminal ID: [ "<< ID <<" ]\n"
       << ld <<"Options ("<< options.size() <<" entries):\n\n";
    for(auto a : options){
        a.print( os, mode, ld + "  " );
        os<<"\n";
    }
}

void GrammarToken::print( std::ostream& os, int mode, const std::string& ld ) const {
    os <<ld<< "[GrammarToken]: type: [ "<< getTypeString( type ) <<" ], ID: "<<id<<
        ( data.empty() ? "" : ("\n"+ld+" Data: \""+data+"\"") ) << "\n";
    if( !children.empty() )
        os <<ld<< " Children: ("<< children.size() <<" entries).\n";
    
    for( auto a : children )
        a.print( os, mode, ld+"- " );
}

std::string GrammarToken::getTypeString( char typ ){
    switch( typ ){
        case GROUP_ONE:         return "GROUP_ONE";
        case GROUP_OPTIONAL:    return "GROUP_OPTIONAL";
        case GROUP_REPEAT_NONE: return "GROUP_REPEAT_NONE";
        case GROUP_REPEAT_ONE:  return "GROUP_REPEAT_ONE";
        case REGEX_STRING:      return "REGEX_STRING";
        case TAG_ID:            return "TAG_ID";
        case ROOT_TOKEN:        return "Option (ROOT_TOKEN)";
    }
    return "INVALID";
}

/*! GBNF TOOLS. 
 * Public functions, called from outside o' this phile.
 */ 
void convertToGbnf(GbnfData& data, std::istream& input){
    //hlogSetFile("grylogz.log", HLOG_MODE_APPEND);
    hlogSetActive(true);

    ParseInput pi( input, data );
    pi.convert();
}

} // namespace gbnf end.

/*
 * Set, Close and Get the current LogFile
 FILE* hlogSetFile(const char* fname, char mode);
 void hlogSetFileFromFile(FILE* file, char mode);
 void hlogCloseFile();
 FILE* hlogGetFile();
 char hlogIsActive();
 void hlogSetActive(char val);
 
 * Write to the LogFile (Printf style) 
 void logf(dbg, const char* fmt, ... );
*/

