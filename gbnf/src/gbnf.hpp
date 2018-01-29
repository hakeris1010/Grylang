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
#include <algorithm>
#include <map>
#include <functional>
#include <vector>
#include <istream>
#include <ostream>
#include <memory>

namespace gbnf{

/*! Non-Terminal Entity structure.
 *  - Also known as the Tag.
 *  - Tag table declares the non-terminals and their IDs.
 *  - Non-Terminals are defined in the Grammar Rule table.
 */ 
struct NonTerminal{
private:
    size_t ID;
public:
    // Mark as "mutable" to allow modification of this field while iterating 
    // in std::set using a reference to this object.
    std::string data;

    NonTerminal( int _ID, const std::string& _data = std::string() ) 
        : ID( _ID ), data( _data ) {}
    NonTerminal( int _ID, std::string&& _data ) 
        : ID( _ID ), data( std::move(_data) ) {} 

    inline size_t getID() const { return ID; }

    // Comparation function.
    bool operator< (const NonTerminal& other) const { 
        return this->ID < other.ID; 
    }
};

/*! Token structure.
 *  - Tokens are in a format defined above.
 *  - Every Grammar Rule option is a Token with a type of ROOT_TOKEN.
 *  - Token structure uses the tree format - with the root token being the option itself.
 *  - Child tokens must be sequentially matched with every incoming 
 *    token-to-match to match the rule.
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
    size_t id = 0;
    std::string data;
    std::vector<GrammarToken> children;

    GrammarToken(){}
    GrammarToken( char _type, int _id, const std::string& _data, 
                  std::initializer_list< GrammarToken >&& _children = {} )
        : type( _type ), id( _id ), data( _data ), children( std::move(_children) )
    {}
    GrammarToken( char _type, int _id, const std::string& _data, 
                  std::vector< GrammarToken >&& _children )
        : type( _type ), id( _id ), data( _data ), children( std::move(_children) )
    {}

    static std::string getTypeString( char typ, bool codeMode = false );
     
    void print( std::ostream& os, int mode=0, const std::string& leader="" ) const ;
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
private:
    size_t ID;

public:
    mutable std::vector<GrammarToken> options; 

    GrammarRule(int _ID) : ID( _ID ) {}
    GrammarRule(int _ID, std::initializer_list< GrammarToken >&& _options)
        : ID( _ID ), options( std::move( _options ) )
    {}
    GrammarRule(int _ID, std::vector< GrammarToken >&& _options)
        : ID( _ID ), options( std::move( _options ) )
    {} 

    void print( std::ostream& os, int mode=0, const std::string& leader="" ) const ;
    inline size_t getID() const { return ID; }

    // Comparation function.
    bool operator< ( const GrammarRule& other ) const {
        return this->ID < other.ID; 
    } 
};

inline std::ostream& operator<< (std::ostream& os, const GrammarRule& rule){
    rule.print( os );
    return os;
}


/*! Whole-File structure.  
 *  This is the structure which holds the whole grammar which is being worked with.
 *
 *  New model - use Sorted Vectors.
 */ 
class GbnfData{
private:
    int lastTagID = 0;
    int lastRuleID = 0;
    bool sorted = false;

    std::vector< NonTerminal > tagTable; 
    std::vector< GrammarRule > grammarTable;     
    std::map< std::string, std::string > paramTable;

public:
    GbnfData(){}

    /*! Constructor from lists of data. 
     *  Used when statically instantiating an object.
     */ 
    GbnfData( int flag, std::initializer_list< NonTerminal >&& tagTbl, 
              std::initializer_list< GrammarRule >&& grammarTbl,
              std::initializer_list< std::pair<std::string, std::string> >&& paramTbl )
        : tagTable( std::move(tagTbl) ), grammarTable( std::move(grammarTbl) ), 
          paramTable( std::move( paramTbl ) ), flags( flag )
    { }

    /*! Construct from GBNF-formatted source file.
     *  @param input - a stream from a file containing GBNF language source.
     */ 
    GbnfData( std::istream& input );
     
    // Last tag getters.
    void print( std::ostream& os, int mode=0, const std::string& leader="" ) const;
    inline int getLastTagID() const { return lastTagID; }
    inline int getLastRuleID() const { return lastRuleID; }
    inline bool isSorted() const { return sorted; }

    // Get const references to tables.
    const auto& tagTableConst() const { return tagTable; }
    const auto& grammarTableConst() const { return grammarTable; }
    const auto& paramTableConst() const { return paramTable; }

    // Get iterators to begin and end.
    auto begin() const { return grammarTable.begin(); }
    auto begin() { return grammarTable.begin(); }
    auto end() const { return grammarTable.end(); }
    auto end() { return grammarTable.end(); } 

    /*! Get tags by index ( ID ).    
     * Const and Non-Const Versions.
     * @return iterator.
     */ 
    inline auto getTag( size_t i ) {
        if( i < tagTable.size() && (tagTable[i].getID() == i) )
            return tagTable.begin() + i;

        return std::lower_bound( tagTable.begin(), tagTable.end(), 
                                 NonTerminal( i ) ); 
    }
    inline auto getTag( size_t i ) const {
        if( i < tagTable.size() && (tagTable[i].getID() == i) )
            return tagTable.begin() + i;

        return std::lower_bound( tagTable.begin(), tagTable.end(), 
                                 NonTerminal( i ) ); 
    }

    /*! Get rules by index ( ID ).    
     * @return iterator pointing to the rule with index i, or if no existing, to end.
     */
    inline auto getRule( size_t i ) {
        if( i < grammarTable.size() && ( grammarTable[i].getID() == i ) )
            return grammarTable.begin() + i;

        return std::lower_bound( grammarTable.begin(), grammarTable.end(), 
                                 GrammarRule( i ) ); 
    }
    inline auto getRule( size_t i ) const {
        if( i < grammarTable.size() && ( grammarTable[i].getID() == i ) )
            return grammarTable.begin() + i;

        return std::lower_bound( grammarTable.begin(), grammarTable.end(), 
                                 GrammarRule( i ) ); 
    }

    /*! Gets param's value string.
     *  @param key - std::string specifying param's name (key)
     *  @param value - reference to std::string to be filled with value data, 
     *                 if the param with that key is present in table.
     *  @return true if found valid existing param.
     */ 
    inline bool getParamValue( const std::string& key, std::string& value ) const {
        auto&& iter = paramTable.find( key );
        if( iter != paramTable.end() ){ // Element exists
            value = iter->second();
            return true;
        }
        return false;
    }

    /*! Deletes a param.
     *  @param key - std::string specifying param's name (key)
     */ 
    inline void deleteParam( const std::string& key ){
        paramTable.erase( key );
    }

    /*! Inserts a param.
     *  @param update - if set, will update the value of the param if it already exists.
     */ 
    inline void insertParam( const std::string& key, const std::string& value, 
                             bool update = false ){
        if( update ){
            auto&& iter = paramTable.find( key );
            if( iter != paramTable.end() ){ // Element exists
                iter->second = value;
                return;
            } 
        }
        // If element doesn't exist of not update, just insert the element.
        paramTable.insert( std::pair<std::string, std::string>( key, value ) );
    } 

    // GrammarRule inserters. Support move semantics.
    inline void insertRule( GrammarRule&& rule ){
        grammarTable.push_back( std::move(rule) );
        sorted = false;
    }
    inline void insertRule( const GrammarRule& rule ){
        grammarTable.push_back( rule );
        sorted = false;
    }
 
    /* New Tag inserters.
     * - ID is assigned automatically, so the vector is always sorted. 
     * @return the ID of newly inserted tag.
     */
    inline size_t insertTag( const std::string& name ){
        lastTagID++;
        tagTable.push_back( NonTerminal( lastTagID, name ) );
        return lastTagID;
    }
    inline size_t insertTag( std::string&& name ){
        lastTagID++;
        tagTable.push_back( NonTerminal( lastTagID, std::move(name) ) );
        return lastTagID;
    }
    inline size_t insertTag( const char* name ){
        lastTagID++;
        tagTable.push_back( NonTerminal( lastTagID, std::string(name) ) );
        return lastTagID;
    }

    size_t getTagIDfromTable( const std::string& name, bool insertIfNotPresent );

    /*! Removers. 
     *  - After removal the sorting order doesn't change.
     */
    inline void removeTag( size_t i ){
        if( i < tagTable.size() && (tagTable[i].getID() == i) )
            tagTable.erase( tagTable.begin() + i );
        else{
            auto&& it = std::lower_bound( tagTable.begin(), tagTable.end(), 
                                          NonTerminal( i ) ); 
            if( it != tagTable.end() )
                tagTable.erase( it );
        }
    }

    inline void removeRule( size_t i ){
        if( i < grammarTable.size() && ( grammarTable[i].getID() == i ) )
            grammarTable.erase( grammarTable.begin() + i );
        else{
            auto&& it = std::lower_bound( grammarTable.begin(), grammarTable.end(), 
                                          GrammarRule( i ) ); 
            if( it != grammarTable.end() )
                grammarTable.erase( it );
        }
    }

    /*! Sorter. Sorts the Grammar Rule Table by ID.
     */
    inline void sort(){
        if( sorted )
            return;
        std::sort( grammarTable.begin(), grammarTable.end() );
        sorted = true;
    }
};

inline std::ostream& operator<< (std::ostream& os, const GbnfData& rule){
    rule.print( os );
    return os;
}

/*! Tools - Converters and Helpers
 *  Contains functions to use when converting to/from EBNF and parsing data.
 *  No matching is done there. Only conversion to/from the gBNF format.
 */

/*! Base class for CodeGenerator implementations
 *  Is launched from private space.
 */
class CodeGenerator_impl{
public:
    virtual ~CodeGenerator_impl(){}

    virtual void outputStart() = 0;
    virtual void outputEnd() = 0;
    virtual void generate(const GbnfData& gb, const std::string& vn ) = 0;
};

/*!
 * Main public CodeGenerator class.
 * - Creates C++ files with construction code of the GBNF structures passed.
 */
class CodeGenerator{
private:
    std::unique_ptr< CodeGenerator_impl > impl;

public:
    CodeGenerator(std::ostream& outp, const std::string& fname);

    void outputStart();
    void outputEnd();
    void generateConstructionCode( const GbnfData& gbData, const std::string& varName, int v=0 );
};

/*! Function uses the EBNF format input from the 'input' stream to fill up the 'data' structure. 
 * @param data - the empty GBNF structure to fill up.
 * @param input - the input stream to read from.
 * @throws runtime_error if fatal error occured.
 */ 
void convertToGbnf(GbnfData& data, std::istream& input, int verbosity = 0);

/*! Function makes a C/C++ header file containing a const GbnfData structure which contains
 *  the gBNF data from the 'data' structure.
 *  @param data - the structure holding the data to move to the file
 *  @param variableName - the name of the const struct to use in that file.
 *  @param output - the output stream to write to. Most likely a file stream.
 *  @throws runtime_error if fatal error occured.
 */ 
void generateCode( const GbnfData& data, std::ostream& output, 
                   const char* variableName, int verbosity = 0 ); 

/*! GBNF Converters. Converts GBNF data to various formats.
 *  - Converts EBNF grammar to BNF, for easier parsing.
 *  - Fixes the left/right recursion (Must be converted to BNF).
 *  @param data - GBNF data structure.
 *  @param preferRightRecursion - if set, conversion to BNF prefers right recursion over left.
 *  @param recursionFixMode - can be set to fix right, left recursion, or nothing (0).
 */ 

const int NO_RECURSION_FIX = 0;
const int FIX_LEFT_RECURSION = 1;
const int FIX_RIGHT_RECURSION = 2;

void convertToBNF( GbnfData& data, bool preferRightRecursion = false, int verbosity = 0 );

void fixRecursion( GbnfData& data, int recursionFixMode = FIX_LEFT_RECURSION, int verbosity = 0 );

}

#endif // GBNF_H_INCLUDED
