#ifndef YUKIPARSE_HPP
#define YUKIPARSE_HPP

#include <gbnf/gbnf.hpp>
#include <memory>

namespace yparse{
class ParserGeneratorImpl;

class ParserGenerator{
    private:
        std::unique_ptr< ParserGeneratorImpl > impl;

    public:
        ParserGenerator(const GbnfData& data, int flags);
        ParserGenerator(GbnfData&& data, int flags);


}

}

#endif // YUKIPARSE_HPP
