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
    void run(){
        SetState(NAS_LOADING);
        while(m_state != NAS_DONE){
            switch (m_state){
                case NAS_LOADING:{
                                     loadPoints();
                                     EndState();
                                     SetState(NAS_WAITING);
                                     msgBuffer.sendCheckPointMessage();
                                     break;
                                 }
                case NAS_LOAD_COMPLETE:{
                                           msgBuffer.barrier();
                                           pumpNetwork();
                                           SetState(NAS_SORT);
                                           break;
                                       }
                case NAS_SORT:{
                                  msgBuffer.barrier();
                                  setUpGlobalMinMax();
                                  setUpPointIds();
                                  sortLocalPoints();
                                  getLocalSample();
                                  setGlobalPivot();
                                  msgBuffer.barrier();
                                  distributePoints();

                                  SetState(NAS_DONE);
                                  break;
                              }
                case NAS_WAITING:
                              pumpNetwork();
                              break;
                case NAS_DONE:
                              break;
            }
        }

    }

    void runControl(){
        SetState(NAS_LOADING);
        while(m_state != NAS_DONE){
            switch(m_state){
                case NAS_LOADING:{
                                     loadPoints();
                                     EndState();
                                     m_numReachedCheckpoint++;
                                     while(!checkpointReached())
                                         pumpNetwork();
                                     //Load complete
                                     SetState(NAS_LOAD_COMPLETE);
                                     msgBuffer.sendControlMessage(APC_SET_STATE,
                                             NAS_LOAD_COMPLETE);
                                     msgBuffer.barrier();
                                     //printPoints();

                                     pumpNetwork();
                                     SetState(NAS_SORT);
                                     break;
                                 }
                case NAS_SORT:{
                                  msgBuffer.sendControlMessage(APC_SET_STATE, NAS_SORT);
                                  msgBuffer.barrier();
                                  setUpGlobalMinMax();
                                  setUpPointIds();
                                  sortLocalPoints();
                                  getLocalSample();
                                  setGlobalPivot();
                                  msgBuffer.barrier();
                                  distributePoints();

                                  SetState(NAS_DONE);
                                  break;
                              }
                case NAS_DONE:
                              break;
            }
        }

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
