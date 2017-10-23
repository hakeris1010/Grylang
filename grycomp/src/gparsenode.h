#ifndef GPARSENODE_H_INCLUDED
#define GPARSENODE_H_INCLUDED

#include <memory>
#include <vector>

namespace gpar{

/*! ParseData class, to store data about lexems/syntems.
 *  - Intended to be extendable.
 */ 

class ParseData
{
    public:
        ParseData(){}
        virtual ~ParseData(){}

        virtual std::ostream& print(std::ostream& os)=0;
};

/*
inline std::ostream& operator<<(std::ostream& os, std::shared_ptr<ParseData> pd){
    return os<<pd.get();
}


inline std::ostream& operator<<(std::ostream& os, ParseData* pd){
    return pd->print(os);
}
*/
/*! Node structure for storing the Parse data. 
 *  - Can be combined into linear or tree-like, or even mesh-based networks.
 *  - Uses std pointers to make even parse loops.
 *  - Extendable - intended to be so!
 */ 

class ParseNode
{
    protected:
        std::vector< std::shared_ptr<ParseNode> > children;
        std::shared_ptr<ParseData> parseData;

        size_t childPosition = 0;

    public:
        ParseNode(){}
        ParseNode(std::shared_ptr<ParseData> data) : parseData(data) {}
        virtual ~ParseNode(){}

        std::shared_ptr<ParseData> getParseData() const {
            return parseData;
        }

        virtual void addChild(std::shared_ptr<ParseNode> child, int pos=-1){
            if(pos >= 0 && pos < children.size())
                children.insert(children.begin()+pos, child);
            else
                children.push_back(child);
        }

        virtual std::shared_ptr<ParseNode> getChildAtPosition(int pos = -1) const {
            if(pos >= 0 && pos < children.size())
                return children[pos];
            return children[ children.size()-1 ];
        }

        virtual std::shared_ptr<ParseNode> getNextChild(){
            if(childPosition < children.size()){
                childPosition++;
                return children[childPosition];
            }
            childPosition = children.size()-1;
            return children[childPosition];
        }
};

}
        
#endif //GPARSENODE_H_INCLUDED
