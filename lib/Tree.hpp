#include <iostream>
#include <algorithm>
#include <string>
#include <queue>
#include "../comm/Messager.hpp"

using namespace std;

template<typename T> class Tree : protected Messager<T> {
    friend class addToQueueHandler<T>;
    friend class sortQueueHandler<T>;

    //buffer array that stores private data
    vector<T> private_data;

public:
    
    void printData() const {
        cout << " ";
        for(auto&& it : private_data) cout << it << " ";
        cout << endl;
    }
};

int main(){
    //define a generic message queue
    Tree<string> testtree;

    //push different types of messages
    testtree.addMessage(new addToQueue<string>(new string("cat")));
    testtree.addMessage(new addToQueue<string>(new string("book")));
    testtree.addMessage(new addToQueue<string>(new string("bus")));
    testtree.addMessage(new sortQueue<string>());
    testtree.addMessage(new addToQueue<string>(new string("apple")));
    testtree.addMessage(new sortQueue<string>());
    testtree.addMessage(new addToQueue<string>(new string("corn")));
    testtree.addMessage(new sortQueue<string>());

    testtree.processMessages();
    return 0;
}
