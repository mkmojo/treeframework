#include "MessageBuffer.hpp"
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
        MessageBuffer<T> msgBuffer;
        std::vector<T> localBuffer;
        std::vector<int> generateOrder;
        std::vector<Node<T> > localArr;
        std::vector<Node<T> > localStruct;
        std::unordered_map<long, int> nodeTable;
        std::unordered_map<int, unordered_set<int> > nodesToSend, nodesToReceive;

        //> user implementation 
        predicate_functional user_predicate;
        generate_functional user_generate;
        combine_functional user_combine;
        evolve_functional user_evolve;
        locate_functional user_locate;

        unsigned numReachedCheckpoint, checkpointSum, maxLevel;
        NetworkActionState state;

        inline bool _checkpointReached() const {
            return numReachedCheckpoint == numProc;
        }

        virtual void _parseControlMessage(int source) {
            ControlMessage controlMsg = msgBuffer.msgBufferLayer.receiveControlMessage();
            switch (controlMsg.msgType) {
                case APC_SET_STATE:
                    _setState(NetworkActionState(controlMsg.argument));
                    break;
                case APC_CHECKPOINT:
                    numReachedCheckpoint++;
                    checkpointSum += controlMsg.argument;
                    break;
                case APC_WAIT:
                    _setState(NAS_WAITING);
                    msgBuffer.msgBufferLayer.barrier();
                    break;
                case APC_BARRIER:
                    assert(state == NAS_WAITING);
                    msgBuffer.msgBufferLayer.barrier();
                    break;
                    //sl15: need default case
            }
        }

        //sl15: currently the return value is not used, but might be later.

        virtual size_t _pumpNetwork() {
            for (size_t count = 0;; count++) {
                int senderID;
                APMessage msg = msgBuffer.msgBufferLayer.checkMessage(senderID);
                switch (msg) {
                    case APM_CONTROL:
                        {
                            _parseControlMessage(senderID);
                            //deal with control message before 
                            //any other type of message
                            return ++count;
                        }
                    case APM_BUFFERED:
                        {
                            std::queue<Message<T>*> msgs;
                            msgBuffer.msgBufferLayer.receiveBufferedMessage(msgs);
                            while (!msgs.empty()) {
                                Message<T>* p = msgs.front();
                                msgs.pop();
                                //sl15: need to assert that the point is local
                                localBuffer.push_back(p->Data); //push into the local buffer
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

        virtual void _setState(NetworkActionState newState) {
            assert(msgBuffer.transmitBufferEmpty());
            state = newState;

            // Reset the checkpoint counter
            numReachedCheckpoint = 0;
            checkpointSum = 0;
        }

        /*
        virtual void _sort() = 0;

        virtual void _load(std::string) = 0;

        virtual void _postLoad() = 0;

        virtual void _assemble() = 0;
        */

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
        Messager() : numReachedCheckpoint(0), checkpointSum(0), maxLevel(0), state(NAS_WAITING), localBound(0), lDependency(true){
        };


        inline bool isEmpty() const { return this->localBuffer.empty() && this->localArr.empty() && this->localStruct.empty(); }

        //sl15: this method needs to be overridden by the subclasses
        virtual int computeProcID() {
            return count % numProc;
        }

        //sl15: this function seems to be unnecessary since loading and sorting are all done in run()
        void add(const T& data) {
            int procID = computeProcID(); //sl15: computeProcID should be implemented by the user
            if (procID == procRank) localBuffer.push_back(data);
            else msgBuffer.addMessage(procID, data);
            count++;
        }
        
        //qqiu: for debug perpose
        void addToProc(int procID, const T& data){
            msgBuffer.addMessage(procID, data);
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
};
