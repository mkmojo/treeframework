#include <iostream>
#include <algorithm>
#include <string>
#include <queue>
#include <stack>
#include "../comm/Messager.hpp"

template<typename T> class Tree : public Messager<T> {
protected:
    //buffer array that stores private data
    std::vector<T> private_data;

    //sl15: required to be implemented by the implementer
    void _load(){ }
    
    void _sort(){ }

    void _assemble(){
        //assert(local_arr->size() <= ndata);
        std::set<int> aux;
        int old_last = -1;

        //define a reference to base class local array to get around the templated protected member non-accessible problem
        std::vector<Node<T> >& localArr = Messager<T>::localArr;
        int new_last = localArr.size()-1;

        while(new_last - old_last > 1){ 
            for(int i = old_last+1; i <= new_last; i++)
                aux.insert(localArr[i].id >> 3);
            old_last = localArr.size()-1;

            for(auto it : aux){
                localArr.push_back(Node<T>(it));
                for(int i = 0; i < localArr.size()-1; i++){
                    if((localArr[i].id) >> 3 == it){
                        localArr.back().childset.insert(std::make_pair(i, localArr[i].id));
                        localArr[i].parent = localArr.size()-1;
                    }
                }
            }
            aux.clear();
            new_last = localArr.size()-1;
        }

        localArr.back().parent = -1;
        for(auto&& it : localArr){
            if(it.childset.empty())
                it.childset.insert(std::make_pair(-1,-1));
        }

        _post_order_walk(&Tree::_add_to_tree);
    }

    //additional implementations by the implementer
    inline void _add_to_tree(int a){ Messager<T>::localStruct.push_back(a); }

    void _post_order_walk(void (Tree::*fun)(int)){
        std::stack<int> aux;
        auto track = new std::vector<NodeOrderSet::iterator>;
        std::vector<Node<T> >& localArr = Messager<T>::localArr;

        for(auto&& it : localArr)
            track->push_back(it.childset.begin());

        int last_visited = -1;
        int current = localArr.size()-1;
        int peek = 0;
        while(!aux.empty() || current != -1){
            if(current != -1){
                aux.push(current);
                current = (*track)[current]->first;
            }else{
                peek = aux.top();
                ++(*track)[peek];
                if((*track)[peek] != localArr[peek].childset.end() && last_visited != ((*track)[peek])->first){
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

public:
    Tree() : Messager<T>() {}
    
    void printData() const {
        std::cout << " ";
        for(auto&& it : private_data) std::cout << it << " ";
        std::cout << std::endl;
    }
};
