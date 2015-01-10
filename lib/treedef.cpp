namespace treedef{
    /** @brief This function contructs a tree node. U: input data type, V: combined type
     *  @param pVar_in pointer to the input data structure
     */
    template <typename U, typename V> 
        TreeNode<U, V>::TreeNode(U* pVar_in):dGen(0),dOwn(0) {
            pVar = pVar_in; //< copy to shared pointer
            pGen = new NodeSet<U, V>; //< initialize generate set
            pOwn = new NodeSet<U, V>; //< initialize owner set
        }


    /** @brief This function destroys a tree node. U: input data type, V: combined type
    */
    template<typename U, typename V> 
        TreeNode<U,V>::~TreeNode(){
            pGen->clear(); //< clear the generate and owner sets
            pOwn->clear();
        }


    /** @brief This function contructs a generate set from a user defined generate list. U: input data type, V: combined type
     *  @param list_in pointer to the input list
     *  @return void
     */
    template<typename U, typename V> 
        void TreeNode<U,V>::getGenSet(NodeList<U, V>* list_in){
            if(list_in->empty()) return;
            for(auto it = list_in.begin(); it != list_in.end(); it++){
                this->pGen->insert(*it); //< insert list nodes into the generate set
                (*it)->pOwn->insert(this); //< insert this into the list nodes' own set
                (*it)->dOwn++; //< increment the owner's counter
            }
            this->dGen = list_in.size(); //< set generate counter to be the length of the list

            list_in->clear(); //< clear the list
            delete(list_in);
        }
    /** @brief This function contructs a generate list for one node. It needs to be specialized by the user. GenID: type of generate, T: input data type, R: combined type
     *  @param u pointer to the node being worked on
     *  @return NodeList
     */
    template <const std::string& GenID, typename T, typename R> NodeList<T,R>* Generate(NodePtr<T, R> u) {
        //> default generate function for tree:
        auto pChild = u->left;
        auto tmplist = new NodeList<T, R>;

        //> push all children into the list
        do{
            tmplist->push_back(pChild);
        } while(pChild.sibling != nullptr);
        return tmplist;
    }


    /** @brief This function combines input data type stored in v into u. It needs to be specialized by the user. ComID: type of combine, T: input data type, R: combined type
     *  @param u pointer to the data structure storing the combined value
     *  @param v pointer to the data structure storing the input value
     *  @return void 
     */
    template <const std::string& ComID, typename T, typename R> void Combine(R* u, T* v) {
        //>default: add all child node's input
        *u += *v;
    }


    /** @brief This function evolves the input data type stored in v based on combined value stored in u. It needs to be specialized by the user. EvoID: type of evolve, T: input data type, R: combined type
     *  @param u pointer to the data structure storing the combined value
     *  @param v pointer to the data structure storing the input value
     *  @return void 
     */
    template<const std::string& EvoID, typename T, typename R> void Evolve(R* u, T* v) {
        //>default: set the new input to be the old combine
        v = u;
    }


    /** @brief This function adapts user defined combine into tree combine. ComID: type of combine, T: input data type, R: combined type
     *  @param u pointer to the data structure storing the combined value
     *  @param v pointer to the data structure storing the input value
     *  @return void 
     */
    template<const std::string& ComID, typename T, typename R> void TreeCombine(NodePtr<T,R> u, NodePtr<T,R> v){
        Combine<ComID, T, R>(u->pCom, v->pVar);
        u->dGen--; //< decrease the gen counter and the own counter
        v->dOwn--; //< a tree node with dOwn = 0 is considered to be removed from the residual tree
    }





    /** @brief This function finds the common ancestor of nodes u and v. T: input data type, R: combined type
     *  @param u pointer to the first node
     *  @param v pointer to the second node
     *  @return NodePtr
     */
    template<typename T, typename R> NodePtr<T,R> BaseTree<T, R>::getCommonAncestor(NodePtr<T,R> u, NodePtr<T,R> v){
        //...
    }


    /** @brief This function creates and inserts a tree node storing input data value of data_in. T: input data type, R: combined type
     *  @param data_in pointer to the first node
     *  @return void
     */
    template<typename T, typename R> void BaseTree<T, R>::Insert(T* data_in){
        //...
    }


    /** @brief This function collapses the basetree from node u up. T: input data type, R: combined type
     *  @param u node considered
     *  @return void
     */
    template<typename T, typename R> void Collapse(NodePtr<T,R> u){
        if(u->dGen != 0) return;
        auto it = u;
        while(it->parent != nullptr && it->parent->dGen <= 1){
            *(it->parent->pCom) = *(it->pCom); //< store the combined value into its parent
            Remove(it);
            it = it->parent;
        }
    }
}
