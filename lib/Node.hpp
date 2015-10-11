#include <set>
#include <map>
#include <unordered_set>
#include <utility>
#include "../comm/Message.hpp"

struct NodeIndex{

    int processorId=-1;
    int index=-1;
    NodeIndex(){}
    NodeIndex(int procId, int arryIndex):processorId(procId), index(arryIndex){}
};

typedef std::unordered_set<int> NodeSet;
typedef std::map<long, int> NodeOrderSet;

struct pairHash{
    inline size_t operator()(const std::pair<int, int>& v) const{
        return v.first*numProc+v.second;
    }
};

template<typename T> class Node : public Message{
    //TODO used when packed to send buffer
    int sDataArr, sRemoteChildren;
    std::vector<T> dataArr;
    //R comval; //sl15: combine type
    NodeSet genset, childindexset;
    NodeOrderSet childset;
    std::unordered_set<std::pair<int, int>, pairHash> remoteChildren;
    long id, parent;
    bool hasData;

    //sl15: all classes that need access to node
    template<typename U> friend class Messager;
    template<typename U> friend class Tree;

    inline void _insert(T data_in){ dataArr.push_back(data_in); }

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

    std::string toStr(){
        if(childset.empty())
            return std::to_string(id);
        else{
            std::string res = std::to_string(id) + "["+ std::to_string(childset.size())+"]"+"( ";
            for(auto &&it: childset){
                res += (std::to_string(it.second) + " ");
            }
            return res + ")";
        }
    }

    size_t getNetworkSize() const{
        return 0;
    }

    size_t serialize(char* buffer){
        return 0;
    }

    size_t unserialize(const char* buffer){
        return 0;
    }

};

