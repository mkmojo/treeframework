#include <iostream>
#include <algorithm>
#include <string>
#include <queue>
#include "../comm/Messager.hpp"

template<typename T> class Tree : protected Messager<T> {
    //buffer array that stores private data
    std::vector<T> private_data;

public:
    void printData() const {
        std::cout << " ";
        for(auto&& it : private_data) std::cout << it << " ";
        std::cout << std::endl;
    }
};
