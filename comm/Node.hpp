#include <set>
#include <unordered_set>
#include <utility>

typedef std::pair<int,int> Pair;
struct pair_compare{
    inline bool operator()(const Pair& lhs, const Pair& rhs) const {
        return lhs.second < rhs.second;
    }
};

typedef std::unordered_set<int> NodeSet;
typedef std::set<Pair, pair_compare> NodeOrderSet;

template<typename T> class Node{
    std::vector<T> dataArr;
    //R comval; //sl15: combine type
    NodeSet genset;
    NodeOrderSet childset;
    int id, parent;
    
    //sl15: all classes that need access to node
    template<typename U> friend class Messager;
    template<typename U> friend class Tree;

    inline void _insert(T data_in){ dataArr.push_back(data_in); }

public:
    Node(int id_in):id(id_in){ }

    Node(T data_in, int id_in):id(id_in),parent(-1){ dataArr.push_back(data_in); }

    ~Node(){ }

    inline int getCount() const { return dataArr.size(); }
};

