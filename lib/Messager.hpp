#include "../comm/MessageBuffer.hpp"
#include "../comm/CommLayer.hpp"
#include "Node.hpp"
#include <map>
#include <unordered_map>
#include <functional>
using namespace std;

template<typename T> class Messager {
    int count = 0;
    protected:
    typedef std::function<bool (const std::vector<T>&) > predicate_functional;
    typedef std::function<NodeSet(const Node<T>&) > generate_functional;
    typedef std::function<void (std::vector<T>&) > combine_functional;
    typedef std::function<void (std::vector<T>&) > evolve_functional;
    typedef std::function<int (const T&, int) > locate_functional;

    int localBound;
    bool lDependency; //dependency flag

    //messages stored in a list of queues
    MessageBuffer msgBuffer;
    std::vector<T> localBuffer;
    std::vector<int> generateOrder;
    std::vector<Node<T> > localArr;
    std::vector<Node<T> > localStruct;
    std::unordered_map<long, int> nodeTable;
    std::unordered_map<int, unordered_set<int> > nodesToSend, nodesToReceive;

    CommLayer *comm = NULL;

    //> user implementation 
    predicate_functional user_predicate;
    generate_functional user_generate;
    combine_functional user_combine;
    evolve_functional user_evolve;
    locate_functional user_locate;

    unsigned numReachedCheckpoint, checkpointSum, maxLevel;

    inline bool _checkpointReached() const {
        return numReachedCheckpoint == numProc;
    }

    virtual size_t _pumpNetwork() {
        for (size_t count = 0;; count++) {
            int senderID;
            APMessage msg = comm->checkMessage(senderID);
            switch (msg) {
                case APM_BUFFERED:
                    {
                        char* pdata;
                        size_t sdata;
                        comm->getRecvBufferAddress(&pdata, &sdata);

                        std::queue<Message*> msgs;

                        int offset = 0;
                        while(offset < sdata){
                            MessageType type = Message::readMessageType(pdata + offset);
                            //create empty new message
                            Message* pNewMessage;
                            switch(type)
                            {
                                case MT_POINT:
                                    pNewMessage = (Message*) (new T);
                                    break;
                                case MT_NODE:
                                    pNewMessage = (Message*) (new Node<T>);
                                    //cout << "DEBUG " + to_string(procRank) + " " + 
                                    //    " got NODE type: " + to_string(MT_NODE) + "\n";
                                    break;
                                default:
                                    cout << "DEBUG " + to_string(procRank) + " " + 
                                        "message type not defined. " + "got: " +
                                        to_string(type) ; 
                                    assert(false);
                                    break;
                            }
                            // Unserialize the new message from the buffer
                            offset += pNewMessage->unserialize(
                                    pdata + offset);
                            msgs.push(pNewMessage);
                        }

                        comm->Irecv();

                        while (!msgs.empty()) {
                            Message* p = msgs.front();
                            msgs.pop();
                            if(p->getType() == MT_POINT)
                                localBuffer.push_back(*(T*)p); 
                            else if(p->getType() == MT_NODE){
                                //receive content in localArr
                                localArr.push_back(*(Node<T>*)p);
                                cout << "DEBUG: " + to_string(procRank) + 
                                    " localArrSize is "+ to_string(localArr.size()) + ". "
                                    + localArr.back().toStr() + "\n";
                            }
                        }
                        break;
                    }
                case APM_NONE:
                    return count;
            }
        }
    }


    inline virtual void _endState() {
        msgBuffer.flush();
    }

    bool getTopologicalSortingOrderHelper(int cur, std::unordered_set<int> visited, std::unordered_set<int> done){
        visited.insert(cur);
        bool hasCycle = false;
        for(auto i : localStruct[cur].genset){
            if(visited.find(i) == visited.end()) 
                hasCycle = hasCycle || getTopologicalSortingOrderHelper(i, visited, done);
            else if(done.find(i) == done.end()) return true;
        }
        generateOrder.push_back(cur);
        done.insert(cur);
        return hasCycle;
    }

    bool getTopologicalSortingOrder(){
        std::unordered_set<int> visited, done;

        for(int i = 0; i < generateOrder.size(); i++){
            if(visited.find(i) == visited.end() && getTopologicalSortingOrderHelper(i, visited, done))
                return true;
        }
        return false;
    }

    public:
    //maxLevel needs to be read in from user input in loadPoint function
    Messager() : numReachedCheckpoint(0), checkpointSum(0), maxLevel(0), localBound(0), lDependency(true){
        comm = &(this->msgBuffer.msgBufferLayer);
    };

    std::string localBuffer_toStr() const {
        std::stringstream ss;
        std::string strLocalBuffer = "";
        for(auto it: localBuffer){
            ss << it.x <<" " << it.y << " "<< it.z <<" "<< it.cellId <<endl;
            strLocalBuffer += ss.str();
            ss.str("");
        }
        return strLocalBuffer;
    }

    std::string localArr_toStr() const {
        std::string res = "";
        for(auto it: localArr){
            res +=  (it.toStr() + " ");
        }
        return res + "\n";
    }

    inline bool isEmpty() const { return this->localBuffer.empty() && this->localArr.empty() && this->localStruct.empty(); }

    //sl15: this method needs to be overridden by the subclasses
    virtual int computeProcID() {
        return count % numProc;
    }

    //sl15: this function seems to be unnecessary since loading and sorting are all done in run()
    void add(T* pdata) {
        int procID = computeProcID(); //sl15: computeProcID should be implemented by the user
        if (procID == procRank) localBuffer.push_back(*pdata);
        else msgBuffer.addMessage(procID, pdata);
        count++;
    }

    //qqiu: for debug perpose
    void addToProc(int procID, T* pdata){
        if (procID == procRank) localBuffer.push_back(*pdata);
        else msgBuffer.addMessage(procID, pdata);
    }

    void addNodeToProc(int procID, Node<T>* pNode){
        //cout << "DEBUG: " + to_string(procRank) + " inside addNodeToProc\n";
        if(procID == procRank){
            cout << "DEBUG: " + to_string(procRank) + " do not add Node to self\n"; 
        }else{
            //cout << "DEBUG: " + to_string(procRank) + " send Node out" + " to " + to_string(procID) + "\n";
            this->msgBuffer.addMessage(procID, pNode);
        }
    }

    void assign(generate_functional generate_in
            , predicate_functional predicate_in
            , combine_functional combine_in
            , evolve_functional evolve_in
            , locate_functional locate_in) {

        user_generate = generate_in;
        user_predicate = predicate_in;
        user_combine = combine_in;
        user_evolve = evolve_in;
        user_locate = locate_in;
    }

    virtual void compute() {
        for (size_t i = 0; i < localBound; i++){
            localStruct[i].genset = user_generate(localStruct[i]);
            generateOrder.push_back(i);
        }

        if(lDependency){
            bool hasCycle = getTopologicalSortingOrder();
            if(hasCycle){ //cycle is detected, print a warning message and quit
                //TODO needs processes to emit error message and quit 
                std::cout << "Generate Graph contains cycle. Terminating Process... " << std::endl;
                exit(-1); //TODO needs better way to quit 
            }
        }

        msgBuffer.msgBufferLayer.barrier();

        for (int i = 0; i < localBound; i++) {
            //call user combine on localStruct[generateOrder[i]]
        }
    }

    size_t redistribute(char* send, int* sendCounts, int* sendDisplacements,
            char* &recv, int* recvCount, int* recvDisplacements){
        int total = 0;
        for (size_t i = 0; i < numProc; i++) {
            total += recvCount[i];
        }
        std::size_t size = total * sizeof(char);
        recv = (char*) malloc(size);
        MPI_Alltoallv(send, sendCounts, sendDisplacements, MPI_BYTE, recv, recvCount, recvDisplacements, MPI_BYTE, MPI_COMM_WORLD);
        return size;
    }

    std::vector<int> getStartsArray(const std::vector<int> &lengths) {
        std::vector<int> starts(numProc, 0);
        for (size_t i = 1; i < lengths.size(); i++) {
            starts[i] = starts[i - 1] + lengths[i - 1];
        }
        return starts;
    }

    std::vector<int> getRecvLengths(const int* allLengths) {
        std::vector<int> rLengths(numProc, 0);
        int step = numProc;
        for (int i = 0; i < numProc; i++) {
            rLengths[i] = allLengths[step * i + procRank];
        }
        return rLengths;
    }

    std::vector<int> scale(const std::vector<T> &data, std::vector<int>& vec) {
        std::vector<int> result(vec.size(),0);
        int i=0;
        for (size_t j = 0; j < vec.size(); j++) {
            for(int k=0;k<vec[j];k++){
                result[j]+=data[i++].getNetworkSize();
            }
        }
        return result;
    }

    void redistribute(std::vector<T> &data, std::vector<int> processorBucketSizes) {
        //get the size in bytes for data each processor is getting
        std::vector<int> lengths = scale(data, processorBucketSizes);

        //exchange among processors how much data it'll be receiving from other processors
        int* allLengths;
        comm->gatherAll(&lengths[0], lengths.size(), allLengths);

        //setup displacement and length arrays (in byte size) for exchanging data among processors
        std::vector<int> rLengths = getRecvLengths(allLengths);
        std::vector<int> starts = getStartsArray(lengths);
        std::vector<int> rStarts = getStartsArray(rLengths);

        //Allocate memory for the buffer to serialize the local data and perform serialization
        size_t dataSize = 0;
        for (auto&& it : data)
            dataSize += it.getNetworkSize();
        char* sendingSerializedData = (char*) (malloc(dataSize));
        size_t total = 0;
        for (auto&& it : data) {
            it.serialize(sendingSerializedData + total);
            total += it.getNetworkSize();
        }

        //clear current data
        data.clear();

        //exchange data
        char* retrievedSerializedData;
        size_t returnSize = redistribute(sendingSerializedData, &lengths[0],
                &starts[0], retrievedSerializedData, &rLengths[0], &rStarts[0]);

        //deserialize the raw data back to T data structures and add to the data array
        total = 0;
        while (total < returnSize) {
            T* d = new T;
            d->unserialize(retrievedSerializedData + total);
            total += d->getNetworkSize();
            data.push_back(*d);
        }

        free(sendingSerializedData);
        free(retrievedSerializedData);
    }
};
