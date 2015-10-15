#include <set>
#include <map>
#include <vector>
#include <unordered_set>
#include <utility>
#include "../comm/Message.hpp"
#include "../lib/DataUtils.hpp"

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
    std::vector<double> testArr;
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
    MessageType mtype = MT_NODE;

    Node(long id_in):id(id_in), hasData(false){ }

    Node(T data_in, int id_in):id(id_in),parent(-1), hasData(true){ dataArr.push_back(data_in); }

    Node(){ }

    ~Node(){ }

    // x : value
    // n : size
    void setTestArr(double x, int n){
        for( int i=0;i<n;i++){
            testArr.push_back(x);
        }
    }

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

    std::string showTestArr(){
        std::string res = "";
        for(auto &&it : testArr){
            res += (std::to_string(it) + " ");
        }
        return res + "\n";
    }

    std::string toStr(){
        if(childset.empty()){
            return std::to_string(id) + "[empty]";
        }
        else{
            std::string res = std::to_string(id) + 
                "["+ std::to_string(childset.size())+"]"+"( ";
            for(auto &&it: childset){
                res += (std::to_string(it.second) + " ");
            }
            return res + ")";
        }
    }

    MessageType getType() const {
        return mtype;
    }


    // in bytes
    size_t getNetworkSize() const{
        return sizeof(mtype) 
            // testArr
            + sizeof(size_t) + (testArr.size() * sizeof(double)) 
            // id
            + sizeof(size_t) + sizeof(getId());
    }

    size_t serialize(char* dest){
        size_t offset = 0;
        offset += data_utils::copyData(dest, &mtype, sizeof(mtype));
        //for testArr
        size_t cnt_size = testArr.size();
        offset += data_utils::copyData(dest+offset, &cnt_size, sizeof(size_t));
        offset += data_utils::copyData(dest+offset, testArr.data(), sizeof(double) * cnt_size);

        //for node_id
        long id_out = getId();
        cnt_size = sizeof(id_out);
        offset += data_utils::copyData(dest+offset, &cnt_size, sizeof(size_t));
        offset += data_utils::copyData(dest+offset, &id_out, sizeof(id_out));

        return offset;
    }

    size_t unserialize(const char* src){
        size_t offset = 0;
        offset += data_utils::copyData(&mtype, src+offset, sizeof(mtype));
        //for testArr
        size_t cnt_size=0;
        offset += data_utils::copyData(&cnt_size, src+offset, sizeof(size_t));
        testArr.clear();
        testArr.resize(cnt_size);
        offset += data_utils::copyData(testArr.data(), src+offset, sizeof(double) * cnt_size);

        //for node_id
        long new_id = 0;
        cnt_size = 0;
        offset += data_utils::copyData(&cnt_size, src+offset, sizeof(size_t));
        offset += data_utils::copyData(&new_id, src+offset, cnt_size);
        setId(new_id);
        return offset;
    }
};
