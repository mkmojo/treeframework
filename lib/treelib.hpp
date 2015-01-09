//Tree Library code
using namespace treedef;

template <typename T, typename R> 
class OctTree : public BaseTree<T, R>{
    //implement specialized methods for OctTree
    public:
        ~OctTree();
};

template <typename T, typename R> 
class RBTree : public BaseTree <T, R>{
    //implement specialized methods for RBTree
};

template <typename T, typename R> NodeList* Generate<"UpwardAggregate">(NodePtr<T,R> u){
    //implement specialized upward aggregate generate set
}

template <typename T, typename R> void Combine<"UpwardAggregate">(R* u, T* v){
    //implement specialized upward aggregate combine
}

template <typename T, typename R> void Evolve<"UpwardAggregate">(R* u, T* v){
    //implement specialized upward aggregate evolve
}
