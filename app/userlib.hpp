//User implementation
template <typename T, typename R> NodeList* Generate<"UserGenerate">(NodePtr<T,R> u){
    auto pChild = u->left;
    auto tmplist = new std::list<NodePtr>();
        
    //user code here
    //insert nodes into tmplist

    return tmplist;
};


template <typename T, typename R> void Combine<"UserCombine1">(R* u, T* v){

};


template <typename T, typename R> void Combine<"UserCombine2">(R* u, T* v){

};


template <typename T, typename R> void Evolve<"UserEvolve1">(R* u, T* v){

};
