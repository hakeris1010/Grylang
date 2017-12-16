#ifndef YUKIPARSE_HPP
#define YUKIPARSE_HPP

#include <gbnf/gbnf.hpp>
#include <memory>

namespace yparse{

class ParserGenerator{
    private:
        std::unique_ptr< ParserGenerator > impl;

    public:
        ParserGenerator( const gbnf::GbnfData& data, int flags );
        ParserGenerator( gbnf::GbnfData&& data, int flags );
        virtual ~ParserGenerator();

};

}

#endif // YUKIPARSE_HPP
