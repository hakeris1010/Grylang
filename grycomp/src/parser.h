#include <memory>
#include <fstream>
//#include <gryltools/blockingqueue.hpp>
#include "gparsenode.h"

namespace gpar{

/*! Base class for all GrylloParsers 
 *  - Contains basic I/O logic, and extendable parse/slash methods.
 *  - Uses GParseTree to store the parsing data
 */

class GParser
{
    public:
        virtual ~GParser(){}

        // One-node methods
        virtual bool hasNext()=0;
        virtual std::shared_ptr<ParseNode> getNextNode()=0;
        //virtual std::shared_ptr<ParseNode> getNextNode(const& ParseData criteria)=0;

        // All-tree methods
        virtual std::shared_ptr<ParseNode> buildParseTree(){ return nullptr; }
        //virtual void parseNodesToQueue(gtools::BlockingQueue& queue){}
};

/*
class GBlockParser : GParser
{
    private:
        std::string
    protected:

    public:
        GBlockParser(

}

class GFileParser : GBlockParser
{
    protected:
        std::ifstream inputFile;

    public:
        
    
}
*/

}
