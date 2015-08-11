#include <set>
#include <map>
#include <unordered_set>
#include <utility>
struct NodeIndex{
	int processorId=-1;
	int index=-1;
	NodeIndex(){}
	NodeIndex(int procId, int arrayIndex):processorId(procId), index(arrayIndex){}
};

//qqiu0806: add in strucutre for index
//node_index {
//  int proc;
//  int index; 
//  long id; //cellId
//}

typedef std::unordered_set<int> NodeSet;
typedef std::map<long, int> NodeOrderSet;
enum NodeType{
    ROOT,
    INTERNAL,
    LEAF
};

template<typename T> class Node{
    std::vector<T> dataArr;
    //R comval; //sl15: combine type
    NodeSet genset, childindexset;
    NodeOrderSet childset;
    long id, parent;
    NodeType type;
    bool hasData;

    //sl15: all classes that need access to node
    template<typename U> friend class Messager;
    template<typename U> friend class Tree;

    inline void _insert(T data_in){ dataArr.push_back(data_in); }
    std::vector<NodeIndex> children;

public:
    Node(long id_in):id(id_in), hasData(false){ }

    Node(T data_in, int id_in):id(id_in),parent(-1), hasData(true){ dataArr.push_back(data_in); }

    ~Node(){ }

    inline int getCount() const { return dataArr.size(); }

    long getId() const{
        return id;
    }

    void setId(int id_in){
        id=id_in;
    }

    int getParent() const{
        return parent;
    }

    void setParent(int parent_in){
        parent=parent_in;
    }

    NodeType getType() const{
        return type;
    }

    void setType(NodeType type_in){
        type=type_in;
    }

    void addChild(NodeIndex childIndex){
        children.push_back(childIndex);
    }

    void clearChildren(){
        children.clear();
    }

    void updateLastChild(NodeIndex childIndex){
        children.pop_back();
        children.push_back(childIndex);
    }

    int numChildren() const{
        return children.size();
    }

    std::vector<NodeIndex>* getChildren() const{
        return (std::vector<NodeIndex>*)&children;
    }
};

