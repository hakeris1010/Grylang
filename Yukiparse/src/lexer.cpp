#include "lexer.hpp"

namespace yparse{

class LexerImpl : public Lexer
{
private:

public:
    LexerImpl( const gbnf::GbnfData& lexicData, std::istream& stream, bool useBQ )
    {}

    LexerImpl( gbnf::GbnfData&& lexicData, std::istream& stream, bool useBQ )
    {}

    void start();
    bool getNextToken( LexicToken& tok );
};


/*=============================================================
 * Public Lexer methods.
 */ 
Lexer::Lexer( const gbnf::GbnfData& lexicData, std::istream& stream, bool useBQ )
    : impl( new LexerImpl( lexicData, stream, useBQ ) )
{}

Lexer::Lexer( gbnf::GbnfData&& lexicData, std::istream& stream, bool useBQ )
    : impl( new LexerImpl( lexicData, stream, useBQ ) )
{}

void Lexer::start(){
    impl->start();
}

bool Lexer::getNextToken( LexicToken& tok ){
    impl->getNextToken( tok );
}

}

