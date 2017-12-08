/*! 
 * gBNF header.
 *  Contains the structures needed for storing and using gBNF data.
 *
 *  GrylloBNF file specs:
 *
 *  File has an extension of .gbnf
 *
 *  Files of size >500 kb are not recommended.
 *
 *  - Bytes 0-3: Magic Number "gBNF"
 *  - Byte  4:   Version number
 *
 *  - Bytes 5-6:   File property flags.
 *    - Bit 0: Tag table present
 *    - Bit 1: Grammar rule table present
 *
 *  - Bytes 7-8: Tag table lenght in bytes.
 *  - Bytes 9-10: Grammar rule table lenght.
 *    (More size bytes can be present if more tables are present) 
 *
 *  - Remaining bytes - Data payload. Present in this order:
 *    1. Tag table
 *    2. Grammar rule table
 *    3. Additional tables.
 *
 *  - Tag table structure (n is variable, rows are terminated by \0).
 *    |   0   |   1   |   2   |  . . . . . . .  |  n-1  |   n   |
 *    [2-byte Tag ID]  [String representation of a tag]    [\0]  
 *
 *  - Grammar rule table structure:      
 *    |   0   |   1   |   2   |   3    |  4  | . . |  i  | . . .  |  j  | . . |  k  | k+1 |
 *    [2-byte Tag ID]  [No. of options]  [Option]   [\0]   . . .   [Option]    [\0]  [\0]
 *
 *    - [2-byte Tag ID]:  The ID of a tag this rule defines.
 *    - [No. of options]: The number of available definition options (in eBNF, separated by |).
 *    - [Option]:         gBNF-defined language option. Options are separated by \0.
 *
 *  - gBNF language option definition:
 *      Similar to eBNF, but format is different. Elements have their Types, which are represented as a
 *      single special ASCII character. These characters need to be escaped if used anywhere else.
 *
 *      These characters are: ? * + " < \
 *
 *    - (? * +) The Group repetition specifier. The wildcards are presented before the group.
 *      Then follows the 2-byte size of the group (number of elements): e.g.:
 *
 *      ?89...   (? is a wildcard, 89 are 2 bytes representing the number of elements, 
 *                  and then follows the elements)
 *
 *    - ( " ) The raw text to be matched in regex-like format. The size of the string is 
 *      a 2-byte word after the ", e.g.:
 *
 *      "89[_a-zA-Z] 
 *
 *    - ( < ) The tag format: <[id-b1][id-b2][size-b1][size-b2][data...], e.g.:
 *
 *      <8904nyaa  ('<' is a type, '89' is 2-byte ID, '04' is size of data string 
 *                  (4 is actually byte with value of 4), 'nyaa' is data associated with tag.).
 *
 *    - Some chars need to be escaped. That's why we use the \ to escape them.
 *      E.g. when ascii value of one of bytes representing size if equal to '\', it needs to 
 *      be escaped with another '\', so written like '\\'.
 */

#ifndef GBNF_H_INCLUDED
#define GBNF_H_INCLUDED

#include <string>
#include <set>
#include <functional>
#include <vector>
#include <cstdint>
#include <istream>
#include <ostream>

namespace gbnf{

/*! Non-Terminal Entity structure.
 *  - Also known as the Tag.
 *  - Tag table declares the non-terminals and their IDs.
 *  - Non-Terminals are defined in the Grammar Rule table.
 */ 
struct NonTerminal{
    short ID;
    std::string data;

    NonTerminal( short _ID, const std::string& _data ) : ID( _ID ), data( _data ) {}

    // Comparation function.
    static bool compare(NonTerminal a, NonTerminal b){ 
        return a.ID < b.ID; 
    }

    // void printConstructionCode( std::ostream& o );
};

/*! Token structure.
 *  - Tokens are in a format defined above.
 *  - Every Grammar Rule option is a Token with a type of ROOT_TOKEN.
 *  - Token structure uses the tree format - with the root token being the option itself.
 *  - Child tokens must be sequentially matched with every incoming token-to-match to match the rule.
 */ 
struct GrammarToken{
    const static char GROUP_ONE         = '1';
    const static char GROUP_OPTIONAL    = '?';
    const static char GROUP_REPEAT_NONE = '*';
    const static char GROUP_REPEAT_ONE  = '+';
    const static char REGEX_STRING      = '\"';
    const static char TAG_ID            = '<';
    const static char ROOT_TOKEN        = 'r';

    char type = 'r';
    short id = 0;
    std::string data;
    std::vector<GrammarToken> children;

    GrammarToken(){}
    GrammarToken( char _type, short _id, const std::string& _data, 
                  const std::initializer_list< GrammarToken >& _children = {} )
        : type( _type ), id( _id ), data( _data ), children( _children )
    {}

    static std::string getTypeString( char typ );
     
    void print( std::ostream& os, int mode=0, const std::string& leader="" ) const ;
    // void printConstructionCode( std::ostream& o );
};

inline std::ostream& operator<< (std::ostream& os, const GrammarToken& tok){
    tok.print( os );
    return os;
}

/*! Grammar rule structure.
 *  ID is the ID of a tag being defined.
 *  Options for rule are stored as a vector of Token Trees (GrammarToken type is ROOT_TOKEN).
 */ 
struct GrammarRule{
    short ID;
    std::vector<GrammarToken> options; 

    GrammarRule(){}
    GrammarRule(short _ID, const std::initializer_list< GrammarToken >& _options)
        : ID( _ID ), options( _options )
    {}

    void print( std::ostream& os, int mode=0, const std::string& leader="" ) const ;
    // void printConstructionCode( std::ostream& o );
};

inline std::ostream& operator<< (std::ostream& os, const GrammarRule& rule){
    rule.print( os );
    return os;
}

/*! Whole-File structure.  
 *  This is the structure which holds the whole grammar which is being worked with.
 */ 
struct GbnfData{
    uint16_t flags = 0;
    std::set<NonTerminal, std::function<bool (NonTerminal, NonTerminal)>> tagTable; 
    std::vector<GrammarRule> grammarTable;

    GbnfData() : tagTable ( NonTerminal::compare ) {}
    GbnfData( uint16_t flg, const std::initializer_list< NonTerminal >& tagTbl, 
                            const std::initializer_list< GrammarRule >& grammarTbl )
        : flags( flg ), tagTable( tagTbl, NonTerminal::compare ), grammarTable( grammarTbl )
    {}

    void print( std::ostream& os, int mode=0, const std::string& leader="" ) const;
    // void printConstructionCode( std::ostream& o );
};

inline std::ostream& operator<< (std::ostream& os, const GbnfData& rule){
    rule.print( os );
    return os;
}

/*! Tools - Converters and Helpers
 *  Contains functions to use when converting to/from EBNF and parsing data.
 *  No matching is done there. Only conversion to/from the gBNF format.
 */

/*! Function uses the EBNF format input from the 'input' stream to fill up the 'data' structure. 
 * @param data - the empty GBNF structure to fill up.
 * @param input - the input stream to read from.
 * @throws runtime_error if fatal error occured.
 */ 
void convertToGbnf(GbnfData& data, std::istream& input);

/*! Function makes a C/C++ header file containing a const GbnfData structure which contains
 *  the gBNF data from the 'data' structure.
 *  @param data - the structure holding the data to move to the file
 *  @param variableName - the name of the const struct to use in that file.
 *  @param output - the output stream to write to. Most likely a file stream.
 *  @throws runtime_error if fatal error occured.
 */ 
void generateCode( const GbnfData& data, std::ostream& output, const char* variableName ); 
 
/*namespace GbnfConstructionCode{
    void printNonTerminal( std::ostream& os, const NonTerminal& a, const auto& param );
    void printGrammarToken( std::ostream& os, const GrammarToken& a, const auto& param );
    void printGrammarRule( std::ostream& os, const GrammarRule& a, const auto& param );
    void printGbnfData( std::ostream& os, const GbnfData& a, const auto& param );
}*/

}

#endif // GBNF_H_INCLUDED
