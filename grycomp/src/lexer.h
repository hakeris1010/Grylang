#ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

#include "parser.h"
#include "gparsenode.h"
#include <istream>
#include <string>

namespace gpar{


// Lexer parser class
class GrylangLexer : GParser
{
private:
    std::istream& input;
    std::shared_ptr<ParseNode> rootNode;
    std::shared_ptr<ParseNode> nextNode;
    bool storeAllNodes = false;

    std::string nextSymbols;

    std::shared_ptr<ParseNode> getNextNode_Priv(bool getNxtNode = true);

public:
    GrylangLexer(std::istream& inputStream);

    // One-node methods
    bool hasNext();
    std::shared_ptr<ParseNode> getNextNode();
    //std::shared_ptr<ParseNode> getNextNode(const& ParseData criteria);

    // Enum constants defining known lexem types.
    enum LexemCode{
         NONE = 0,
         LexicERROR = -1,
         FatalERROR = -2,

         KEYWORD = 1,
         IDENT   = 2,
         INTEGER = 3,
         CHAR    = 4,
         FLOAT   = 5,
         STRING  = 6,
         COMMENT = 7,
         OPERATOR = 8
    };

    const std::vector<std::string> GKeywords = {
        "char" , "int" , "int16" , "int32" , "int64" , "float" , "double" , "void" , "var"
        , "fun" , "class" , "private" , "protected" , "public" , "extends" , "implements"
        , "const" , "volatile" , "if" , "else" , "switch" , "case" , "default" , "while"
        , "for" , "foreach" , "in" , "break" , "goto" , "return"
    };
};

// Lexer ParseData class
class LexicParseData : ParseData
{
public:
    LexicParseData(){}
    LexicParseData(const LexicParseData& pd) : code(pd.code), data(pd.data) {}
    LexicParseData(GrylangLexer::LexemCode _code, const std::string& _data) : code(_code), data(_data) {}

    virtual std::ostream& print(std::ostream& os){
        return os<<"Code: "<< this->code <<", Data: "<< this->data;
    }

    GrylangLexer::LexemCode code = GrylangLexer::LexemCode::NONE;
    std::string data;
};

}

#endif //LEXER_H_INCLUDED

