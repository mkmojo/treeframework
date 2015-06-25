#include <iostream>
#include <algorithm>
#include <string>
#include <queue>
#include "../comm/Messager.hpp"

template<typename T> class Tree : protected Messager<T> {
    //buffer array that stores private data
    std::vector<T> private_data;
    std::string filename;

    //sl15: this needs to be implemented
    void _load(){

    }
    
    void _sort(){

    }

public:
    Tree(std::string filename_in) : Messager<T>(), filename(filename_in) { }
    
    void printData() const {
        std::cout << " ";
        for(auto&& it : private_data) std::cout << it << " ";
        std::cout << std::endl;
    }
};
