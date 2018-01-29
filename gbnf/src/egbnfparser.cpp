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
#include "gbnf.hpp"
extern "C" {
    #include <gryltools/hlog.h>
}

namespace gbnf{
    
struct ParseState{
    size_t line = 0;
    size_t pos = 0;
};

class ParseInput{
private:
    int debugMode = 0;

    gtools::StackReader reader;
    GbnfData& data;

    ParseState ps;
    std::string tempData;

    template<typename... Args>
    inline void logf( bool debug, int logPriority, const char* message, Args&&... args ) const;

    inline void throwError( const std::string& message ) const; 
    inline void updateLineStats( const std::string& str );
    inline void updateLineStats( char c );

    //short getTagIDfromTable( const std::string& name, bool insertIfNotPresent );
    void getTagName( std::string& str );
    int  parseGrammarToken( GrammarToken& tok, int recLevel = 1, char endChar = '}' );
    bool parseGrammarOption( GrammarToken& tok );
    void parseGrammarRule();
 
public:
    ParseInput( std::istream& is, GbnfData& dat, int debMode = 0 ) 
        : debugMode( debMode ), reader( is ), data( dat )
    {}

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
 *  @param dewit - (Do it) - whether to output a log message or not.
 *  @param verboseness - level of verbosity (higher means more verbose). 
 */ 
template<typename... Args>
inline void ParseInput::logf( bool dewit, int verboseness, 
            const char* message, Args&&... args ) const {
    if(!dewit || (verboseness > debugMode) )
        return;
    hlogf( message, std::forward<Args>( args )... );
}

/*! Can be used to find NonTerminal tag's ID from it's name, 
 *  or to insert into a table a new NonTerminal with name = name. ID ass'd automatically.
 *  @return the ID of the tag got or inserted.
 */ 
size_t GbnfData::getTagIDfromTable( const std::string& name, bool insertIfNotPresent ){
    // Search by iteration, because we can't search for string sorted.
    for(const auto& t : tagTable){
        if(t.data.compare( name ) == 0)
            return t.getID();
    }
    // If reached this point, element not found. Insert new NonTerminal Tag if flag specified.
    if(insertIfNotPresent)
        return insertTag( name );

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
 *  Tag format: <\s*\w+\s*>
 *  @param str - a buffer to which to write a tag.
 */ 
void ParseInput::getTagName( std::string& str ){
    char c = 0;

    if( reader.peekChar() == '<' ){ // Starts with a '<' - skip it.
        reader.getChar( c );
        reader.skipWhitespace(gtools::StackReader::SKIPMODE_SKIPWS_NONEWLINE, ps.line, ps.pos);
    }

    while( reader.getChar( c ) ){
        updateLineStats( c );

        if( c == '>' ){ // End
            if( str.empty() )
                throwError( "Tag is empty!" );
            break; // if string is not empty, break and return good.    
        }
        // Tag chars: [a-zA-Z_]
        else if( !std::isalnum( c ) && c != '_' ){
            // Got whitespace (not endline). After space only end can be reached.
            if( std::isspace( c ) && c != '\n' ){
                reader.getChar( c, gtools::StackReader::SKIPMODE_SKIPWS_NONEWLINE, 
                                ps.line, ps.pos );
                // Check if end. Only end is allowed.
                if( c == '>' ){ 
                    if( str.empty() )
                        throwError( "Tag is empty!" );
                    break; // if string is not empty, break and return good.    
                } 
                else
                    throwError( "Wrong character in a tag!" );
            }
            else
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
    const bool dbg = true;

    const static int END_OF_STREAM             = 2;
    const static int RECURSIVE_ENDCHAR_REACHED = 1;
    //const static int NORMAL_SUCCESS            = 0;

    std::string recs( recLevel, ' ' );
    logf(dbg, 2, "%s[parseGrammarToken(_,_,\'%c\']\n", recs.c_str(), endChar);

    // Check if token start character.
    char c;
    std::string buff;

    // Get the next character, skipping any whitespaces.
    if( !reader.getChar( c, gtools::StackReader::SKIPMODE_SKIPWS, ps.line, ps.pos ) )
        return END_OF_STREAM;

    // Check all valid token start characters.
    // Non-Terminal 
    if( c == '<' ){
        logf(dbg, 2, "%sTag recognized... \n", recs.c_str());

        buff.reserve(32); // Reserve at least N chars for the upcoming tagname.
        getTagName( buff, true );

        logf(dbg, 2, "%sGot Name:%s\n", recs.c_str(), buff.c_str());

        tok.type = GrammarToken::TAG_ID;
        tok.id = data.getTagIDfromTable( buff, true );
    }
    // Regex-String
    else if( c == '\"' ){
        logf(dbg, 2, "%sString recognized... \n", recs.c_str());

        tok.type = GrammarToken::REGEX_STRING;
        register bool afterEscape = false;

        while( reader.getChar( c ) ){
            updateLineStats( c );

            if( c=='\"' && !afterEscape )
                break; 

            // If any character, add it to our string.
            tok.data.push_back( c ); 

            // Handle escapes
            if( c == '\\' ){
                afterEscape = true;
                continue;
            }

            if(afterEscape)
                afterEscape = false;
        }
        if( c != '\"' ) // Wrong end
            throwError( "String hasn't ended!" );

        logf(dbg, 2, "%sData: \"%s\"\n", recs.c_str(), tok.data.c_str());
    }
    // Group. Several repeat types included.
    else if( c == '{' ){
        // Allocate a token.
        GrammarToken temp;

        logf(dbg, 2, "%sRecursive Group start recognized. Getting childs...\n", recs.c_str());

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

        logf(dbg, 2, "%sGroup ended. Group type: [ %c ], Child Count: %d\n", 
               recs.c_str(), tok.type, tok.children.size() );
    }
    // Check if current character is a recursive group end character. 
    else if( c == endChar ){
        logf(dbg, 2, "%sRecursive Group ended. End char: \'%c\'\n\n", recs.c_str(), c );

        return RECURSIVE_ENDCHAR_REACHED;  // Returned from a recursion.
    }
    // Check if comment start - skip until da end.
    else if( c == '#' ){
        reader.skipUntilChar( '\n' );
    }
    // Other character - just Throw an Error and be happy.
    else 
        throwError( "Wrong token start symbol: "+std::string(1, c) );

    //logf(dbg,"%sSuccessfully extracted a token!\n", recs.c_str());
    logf(dbg, 2, "\n");
    return 0; //NORMAL_SUCCESS; // Success.
}

/*! Get a Grammar Option (Root Token).
 *  - Consists of several child tokens, of which some can be nested or even recursive.
 *  @param tok - a reference to a ready GrammarToken structure.
 *  @return - true if expecting more options, false if otherwise (rule ended or stream ended)
 */ 
bool ParseInput::parseGrammarOption( GrammarToken& tok ){
    const bool dbg = true;

    // Option is a ROOT Token, assign this type.
    tok.type = GrammarToken::ROOT_TOKEN;
    char c;

    logf(dbg, 2, "[parseGrammarOption(_)]\n");

    while( reader.getChar( c, gtools::StackReader::SKIPMODE_SKIPWS, ps.line, ps.pos ) ){
        // Check if option end (a pipe symbol) - just return true, and expect next option.
        if( c == '|' )
            return true;

        // Whole rule end - return false, don't expect more options. 
        else if( c == ';' )
            return false;

        // Check if comment start - skip until endline.
        else if( c == '#' ){
            reader.skipUntilChar('\n');
            continue;
        }

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
 *  MUST start reading at the position of Tag Start ('<').
 *
 *  - Identifying Tag can start with a special character.
 *     If it is, it can specify these things.
 *    1. '#' - Param tag. Expect param value next.
 *    2. '!' - First rule. 
 *
 *  - Function parses the rule, and puts it directly into GbnfData structure.
 */ 
void ParseInput::parseGrammarRule(){
    const bool dbg = true;
    const int SPECMODE_FIRST_RULE = 1;
    const int SPECMODE_PARAM      = 2;

    std::string tmp;
    tmp.reserve(256);

    logf(dbg, 1, "[parseGrammarRule(_)]... ");
    logf(dbg, 2, "\nGetting TagName... \n");
    
    // 1. Get the tag start '<' and the special char, if there exists one.
    // Then get the first tag (the NonTerminal this rule defines), and it's ID.
    char c = 0;
    reader.getChar(c);
    if( c != '<' )
        throwError( "Wrong symbols at rule's start!" );

    // Check for special.
    reader.skipWhitespace( gtools::StackReader::SKIPMODE_SKIPWS_NONEWLINE );
    char c = reader.peekChar();

    int specMode = 0;
    if( c == '!' )
        specMode = SPECMODE_FIRST_RULE;
    else if( c == '#' )
        specMode = SPECMODE_PARAM;

    // Get tag name string.
    getTagName( tmp );

    // Get the definition-assignment operator (::==, ::=, :==, :=).
    char ass[4];
    reader.getString( ass, 4, gtools::StackReader::SKIPMODE_SKIPWS, ps.line, ps.pos );

    if( !strncmp( ass, "::==", 4 ) ) 
        {}
    else if( !strncmp( ass, "::=", 3) || !strncmp( ass, ":==", 3) )
        reader.putChar( ass[3] );
    else if( !strncmp( ass, ":=", 2) )
        reader.putString( ass+2, 2 );  
    else if( !strncmp( ass, "=",  1) )
        reader.putString( ass+1, 3 );   
    else
        throwError("No Def-Assignment operator on a rule"); 

    // Parameter mode. Harvest only one token.
    if( specMode == SPECMODE_PARAM ){
        
    }
    // If Grammar Rule is expected, get options.
    else{
        // Add to table if not present. 
        size_t rID = data.getTagIDfromTable( std::move(tmp), true ); 
        
        // Create a new grammar rule, which we'll fill in next steps.
        GrammarRule rule( rID );
        if( specMode == SPECMODE_FIRST_RULE )
            rule.setProperty( GrammarRule::FIRST_RULE );

        logf(dbg, 1, " TagName: %s, ID: %d \n", data.getTag( rule.getID() )->data, rule.getID());
        logf(dbg, 2, "Getting assignment OP...\n");

        // Get options (ROOT type tokens), one by one, in a loop
        bool areMore = true;

        logf(dbg, 2, "Getting Options in a Loop...\n\n");

        while( areMore ){
            rule.options.push_back( GrammarToken() );
            GrammarToken& tok = rule.options[ rule.options.size()-1 ];

            areMore = parseGrammarOption( tok ); 

            // Accept only non-empty option tokens.
            if( tok.children.empty() )
                rule.options.pop_back();

            logf(dbg, 2, "Got Option: Count of Childs: %d\n\n", tok.children.size());
        }

        // We've parsed a rule. All options are parsed.
        logf(dbg, 1, " Option count: %d\n\n", rule.options.size());
        logf(dbg, 2, "============================\n\n");

        // Put the rule into the grammar table using move semantics.
        data.insertRule( std::move( rule ) ); // Best part: std::move :D
    }
}

// Convert EBNF to GBNF.
void ParseInput::convert(){
    const bool dbg = true;

    char c;
    while( true ){
        // Get first non-whitespace character.
        if( !reader.getChar( c, gtools::StackReader::SKIPMODE_SKIPWS, ps.line, ps.pos ) ) 
            break;

        // Check possibilities: comment, rule.
        if(c == '#'){ // Comment start
            logf(dbg, 2, "Comment started. Skipping until \\n...");

            // Ignore chars until the endline, or Over 9000 of them have been read.
            if( !reader.skipUntilChar('\n') )
                break; 
            //ps.line++;
        }
        else if(c == '<'){ // Rule start. Get the rule and put into the table.
            reader.putChar( c );
            
            logf(dbg, 2, "Grammar Rule started. Getting it...\n");

            // Parse the next grammar rule, and put it direcly into GBNF Data structure.
            parseGrammarRule();
        }
        else // If other non-whitespace character occured between rules and comments, error.  
            throwError(" : Wrong start symbol!" );
    }

    // At the end, sort the GBNF rules by their IDs.
    data.sort();
}

/*! GBNF Structure Printers. 
 *  Prints the GBNF Data to stream passed.
 */
void GbnfData::print( std::ostream& os, int mode, const std::string& ld ) const {
    os << ld <<"GBNFData in 0x"<<this<<"\n"<<ld<<" Flags:"<<flags<<"\n"
       << ld <<" TagTable ("<< tagTable.size() <<" entries):\n";
    for(auto a : tagTable)
        os<<ld<<" [ "<<a.getID()<<" ]: "<<a.data<<"\n";

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

std::string GrammarToken::getTypeString( char typ, bool codeMode ){
    switch( typ ){
        case GROUP_ONE:         return (codeMode ? "GrammarToken::GROUP_ONE"        : "GROUP_ONE"           );
        case GROUP_OPTIONAL:    return (codeMode ? "GrammarToken::GROUP_OPTIONAL"   : "GROUP_OPTIONAL"      );
        case GROUP_REPEAT_NONE: return (codeMode ? "GrammarToken::GROUP_REPEAT_NONE": "GROUP_REPEAT_NONE"   );
        case GROUP_REPEAT_ONE:  return (codeMode ? "GrammarToken::GROUP_REPEAT_ONE" : "GROUP_REPEAT_ONE"    );
        case REGEX_STRING:      return (codeMode ? "GrammarToken::REGEX_STRING"     : "REGEX_STRING"        );
        case TAG_ID:            return (codeMode ? "GrammarToken::TAG_ID"           : "TAG_ID"              );
        case ROOT_TOKEN:        return (codeMode ? "GrammarToken::ROOT_TOKEN"       : "Option (ROOT_TOKEN)" );
    }
    return (codeMode ? "0" : "INVALID");
}

/*! GBNF TOOLS. 
 * Public functions, called from outside o' this phile.
 */ 
void convertToGbnf(GbnfData& data, std::istream& input, int debugMode){
    //hlogSetFile("grylogz.log", HLOG_MODE_APPEND);
    hlogSetActive( debugMode ? true : false );

    ParseInput pi( input, data, debugMode );
    pi.convert();
}

} // namespace gbnf end.


