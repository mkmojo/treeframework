//User implementation
template <typename T, typename R> NodeList* Generate<"UserGenerate">(NodePtr<T,R> u){ 
    auto pChild = u->left;
    auto tmplist = new std::list<NodePtr>();
        
    //user code here
    //insert nodes into tmplist

    return tmplist;
};

template <typename T> void Combine<"UserCombine">(NodePtr<T,R> u){
    if(u->genset == nullptr) 
        throw UnknownGenSetException; //exception: generate not specified

    //user code here
    //get nodes from genset of u, combine their values and store back to u->pCom
};
