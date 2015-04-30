#include <iostream>
#include <fstream>
#include <vector>
#include <functional>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <sstream>
#include <stack>
#include <bitset>
#include <cassert>

typedef std::pair<int,int> Pair;

struct pair_compare{
    inline bool operator()(const Pair& lhs, const Pair& rhs) const {
        return lhs.second < rhs.second;
    }
};

typedef std::unordered_set<int> NodeSet;
typedef std::set<Pair, pair_compare> NodeOrderSet;

template<typename T, typename R> class Node{
    std::vector<T> data_arr;
    R comval;
    NodeSet genset;
    NodeOrderSet childset;
    int id, parent;
    
    template<typename U, typename V> friend class Tree;

    inline void _insert(T data_in){
        data_arr.push_back(data_in);
    }

public:
    Node(int id_in):id(id_in){ }

    Node(T data_in, int id_in):id(id_in),parent(-1){
        data_arr.push_back(data_in);
    }

    ~Node(){ }

    inline int getCount() const { return data_arr.size(); }
};

enum Verbosity {NVerbose, Verbose};

template<typename T, typename R> class Tree{
    typedef std::function<bool (const std::vector<T>&)> predicate_functional;
    typedef std::function<NodeSet (const Node<T,R>&)> generate_functional;
    typedef std::function<void (R&, std::vector<T>&)> combine_functional;
    typedef std::function<void (std::vector<T>&, R&)> evolve_functional;
    typedef std::function<int (const T&, int)> locate_functional;

    //> bookkeeping info
    int ndim, ndata, maxlevel;
    Verbosity verbosity;

    //> local storage
    std::vector<std::vector<int> > leveldata;
    std::vector<int> local_tree;
    std::vector<T>* buffer_arr;
    std::vector<Node<T,R> >* local_arr;
    std::unordered_map<int, int> node_table;

    //> user implementation 
    predicate_functional user_predicate;
    generate_functional user_generate;
    combine_functional user_combine;
    evolve_functional user_evolve;
    locate_functional user_locate;

    //> returns a local node based on its index
    inline Node<T,R>& _local(int a) const { return local_arr->at(a); }

    void _preload(std::string filepath){
        std::ifstream fs(filepath.c_str());
        std::string line;
        bool datastart = false;

        while(std::getline(fs, line)){
            std::istringstream iss(line);
            std::string params;
            if(!datastart){
                iss >> params;
                if(params == "NDIM" && !ndim) iss >> ndim;
                if(params == "MAX_LEVEL" && !maxlevel) iss >> maxlevel;
                if(params == "DATA_TABLE"){
                    datastart = true;
                    continue;
                }
            }else{
                buffer_arr->push_back(T(iss));
                ndata++;
            }
        }
        if(verbosity == Verbose) std::cout << "preload done! " << ndata << std::endl;
    }

    void _flush_buffer(){
        for(auto it : *buffer_arr){
            if(verbosity == Verbose) std::cout << it.x << ", " << it.y << ", " << it.z << std::endl;
            int tmp = user_locate(it, maxlevel);
            if(verbosity == Verbose) std::cout << "node = " << (std::bitset<6>)tmp << std::endl;
            auto itt = node_table.find(tmp);
            if(itt != node_table.end()){
                if(verbosity == Verbose) std::cout << "existing node" << std::endl;
                _local((*itt).second)._insert(it);
            }else{
                if(verbosity == Verbose) std::cout << "new node" << std::endl;
                local_arr->push_back(Node<T,R>(it, tmp));
                node_table[tmp] = local_arr->size()-1;
            }
        }
        if(verbosity == Verbose) std::cout << "flush done! " << local_arr->size() << std::endl;
        buffer_arr->clear();
    }

    inline void _add_to_tree(int a){ local_tree.push_back(a); }

    void _post_order_walk(void (Tree::*fun)(int)){
        std::stack<int> aux;
        auto track = new std::vector<NodeOrderSet::iterator>;
        for(auto&& it : *local_arr)
            track->push_back(it.childset.begin());

        int last_visited = -1;
        int current = local_arr->size()-1;
        int peek = 0;
        while(!aux.empty() || current != -1){
            if(current != -1){
                aux.push(current);
                current = (*track)[current]->first;
            }else{
                peek = aux.top();
                ++(*track)[peek];
                if((*track)[peek] != _local(peek).childset.end() && last_visited != ((*track)[peek])->first){
                    current = (*track)[peek]->first;
                }else{
                    (this->*fun)(peek);
                    last_visited = aux.top();
                    aux.pop();
                }
            }
        }
        delete track;
    }

    void _assemble(){
        assert(local_arr->size() <= ndata);
        std::set<int> aux;
        int old_last = -1;
        int new_last = local_arr->size()-1;

        while(new_last - old_last > 1){ 
            for(int i = old_last+1; i <= new_last; i++)
                aux.insert(_local(i).id >> 3);
            old_last = local_arr->size()-1;

            for(auto it : aux){
                local_arr->push_back(Node<T,R>(it));
                for(int i = 0; i < local_arr->size()-1; i++){
                    if((_local(i).id) >> 3 == it){
                        local_arr->back().childset.insert(std::make_pair(i, _local(i).id));
                        _local(i).parent = local_arr->size()-1;
                    }
                }
            }
            aux.clear();
            new_last = local_arr->size()-1;
        }

        local_arr->back().parent = -1;
        for(auto&& it : *local_arr){
            if(it.childset.empty())
                it.childset.insert(std::make_pair(-1,-1));
        }

        _post_order_walk(&Tree::_add_to_tree);
    }

    void _assemble(T t_in){
        Node<T, R> mynode(t_in);
        local_arr->push_back(mynode);
    }

    //> wrapper function for user generate
    NodeSet _generate(const Node<T,R>& curnode){
        NodeSet genset;
        NodeSet tmp = user_generate(curnode);

        for(auto it = tmp.begin(); it != tmp.end(); it++){
            if(user_predicate(_local(*it).data_arr)) genset.insert(*it);
        }
        return genset;
    }

    //> wrapper function for user combine
    void _combine(Node<T,R>& curnode){
        user_combine(curnode.comval, curnode.data_arr);
    }

    //> wrapper function for user evolve
    void _evolve(Node<T,R>& curnode){
        user_evolve(curnode.comval, curnode.data_arr);
    }

    void _generate_leveldata(){
        std::vector<int> firstlevel;
        firstlevel.push_back(0);
        leveldata.push_back(firstlevel);
    }

public:
    Tree(Verbosity verbose_in = NVerbose)
        : verbosity(verbose_in){
        maxlevel = 0; //comes from tree construction procedure
        ndata = 0; ndim = 0;
        local_arr = new std::vector<Node<T,R> >;
        buffer_arr = new std::vector<T>;
    }

    virtual ~Tree(){
        clear();
    }

    void assign(generate_functional generate_in
        , predicate_functional predicate_in
        , combine_functional combine_in
        , evolve_functional evolve_in
        , locate_functional locate_in){

        user_generate = generate_in;
        user_predicate = predicate_in;
        user_combine = combine_in;
        user_evolve = evolve_in;
        user_locate = locate_in;
    }

    void build(std::string filepath){
        _preload(filepath);
        //sorting called here
        _flush_buffer();
        _assemble();
        if(verbosity == Verbose) print(std::cout);
    }

    void clear(){
        buffer_arr->clear();
        local_arr->clear();
        local_tree.clear();
    }

    void compute(){
    /*
        for(int i = 0; i < local_arr->size(); i++)
            _local(i).genset = _generate(_local(i));
        
        _generate_leveldata();

        for(int level = maxlevel; level >= 0; level--){
            for(int i = 0; i < leveldata[level].size(); i++)
                _combine(_local(leveldata[level][i]));
        }
    */
    }

    std::string getLocalTree() const {
        std::string result("");
        for(auto it = local_tree.begin(); it != local_tree.end(); it++){
            auto bset = std::bitset<7>(_local(*it).id);
            result = result + bset.to_string() + "*";
        }
        if(!result.empty()) result.pop_back();
        return result;
    }

    std::string getLinearTree() const {
        std::string result("");
        for(auto it = local_arr->rbegin(); it != local_arr->rend(); it++){
            auto curr_bset = std::bitset<7>((*it).id);
            result = result + curr_bset.to_string() + "(" + std::to_string((*it).getCount()) + ")" + "[";
            for(auto itt = (*it).childset.begin(); itt != (*it).childset.end(); itt++){
                if((*itt).first != -1){
                    auto child_bset = std::bitset<7>(_local((*itt).first).id);
                    result = result + child_bset.to_string() + ",";
                }
            }
            if(result.back() == ',') result.pop_back();
            result = result + "];";
        }
        if(!result.empty()) result.pop_back();
        return result;
    }

    inline bool isEmpty() const { return buffer_arr->empty() && local_arr->empty() && local_tree.empty(); }

    void print(std::ostream& stream) const {
        std::string linear_str = getLinearTree();
        std::string tree_str = getLocalTree();
        stream << linear_str << std::endl;
        stream << tree_str << std::endl;
    }
};
