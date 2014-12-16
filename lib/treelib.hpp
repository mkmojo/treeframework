//Tree Library code
using namespace treedef;

template class<typename T> OctTree : public BaseTree{
    //implement specialized methods for OctTree
}

template class<typename T> RBTree : public BaseTree{
    //implement specialized methods for RBTree
}

template <typename T, typename R> NodeList* Generate<"UpwardAggregate">(NodePtr<T,R> u){
    //implement specialized upward aggregate generate set
}

template <typename T, typename R> void Combine<"UpwardAggregate">(NodePtr<T,R> u){
    //implement specialized upward aggregate combine
}
