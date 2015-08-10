#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <queue>
#include <stack>
#include <set>
#include <bitset>
#include <math.h>
#include <map>

#include "../comm/Messager.hpp"
#include "DataUtils.hpp"

using namespace std;

struct OctreePoint {
    double x,y,z;
    long cellId;

    virtual void save(void* dest)=0;
    virtual size_t load(const void* src)=0;
    virtual size_t size()=0;
    virtual void free()=0;

    OctreePoint():x(0), y(0), z(0), cellId(0){}

    OctreePoint(std::istringstream& ss):cellId(0){
        ss >> x >> y >> z;
    }

    virtual void serialize(char* dest){
        size_t total=0;
        total+=data_utils::copyData(dest+total, &x, sizeof(x));
        total+=data_utils::copyData(dest+total, &y, sizeof(y));
        total+=data_utils::copyData(dest+total, &z, sizeof(z));
        total+=data_utils::copyData(dest+total, &cellId, sizeof(cellId));
        save(dest+total);
    }

    virtual size_t unserialize(const char* src){
        int total=0;
        total+=data_utils::copyData(&x, src+total, sizeof(x));
        total+=data_utils::copyData(&y, src+total, sizeof(y));
        total+=data_utils::copyData(&z, src+total, sizeof(z));
        total+=data_utils::copyData(&cellId, src+total, sizeof(cellId));
        total+=load(src+total);
        return total;
    }

    virtual size_t getSize() const{
        return 3*sizeof(double)+sizeof(long);
    };
};

template<typename T> class Tree: public Messager<T> {
    private:
        CommLayer<T> comm; //reference variable for the ComLayer defined in parent class
        map<int ,NodeIndex> parentIndexMap;


        std::vector<Node<T> > localArr;
        std::unordered_map<long, int> nodeTable;
        std::vector<long> boundry_array; //used to save borrowed nodes

        int ndim=0, maxlevel=0;
        double globalMinX=0, globalMaxX=0;
        double globalMinY=0, globalMaxY=0;
        double globalMinZ=0, globalMaxZ=0;


        static bool cmpPoint(const OctreePoint &p1, const OctreePoint &p2) {
            return p1.cellId < p2.cellId;
        }


        //Find local min and max for each dimension
        //Find global min and max through MPI ALLreduce
        //distribute the result
        void setUpGlobalMinMax() {
            //TODO: what do we do with a processor that does not
            //have any data?
            assert(this->localBuffer.size() > 0);
            OctreePoint &cntPoint = this->localBuffer[0];
            double minX(cntPoint.x), maxX(cntPoint.x);
            double minY(cntPoint.y), maxY(cntPoint.y);
            double minZ(cntPoint.z), maxZ(cntPoint.z);

            for (unsigned i = 1; i < this->localBuffer.size(); i++) {
                OctreePoint &cntPoint = this->localBuffer[i];
                //For current smallest cord
                if (cntPoint.x < minX) {
                    minX = cntPoint.x;
                }
                if (cntPoint.y < minY) {
                    minY = cntPoint.y;
                }
                if (cntPoint.z < minZ) {
                    minZ = cntPoint.z;
                }

                //For current largest cord
                if (cntPoint.x > maxX) {
                    maxX = cntPoint.x;
                }
                if (cntPoint.y > maxY) {
                    maxY = cntPoint.y;
                }
                if (cntPoint.z > maxZ) {
                    maxZ = cntPoint.z;
                }
            }

            //set global dimension
            globalMinX = comm.reduce(minX, MIN);
            globalMinY = comm.reduce(minY, MIN);
            globalMinZ = comm.reduce(minZ, MIN);
            globalMaxX = comm.reduce(maxX, MAX);
            globalMaxY = comm.reduce(maxY, MAX);
            globalMaxZ = comm.reduce(maxZ, MAX);
        }

        long computeKey(unsigned int x, unsigned int y, unsigned z) {
            // number of bits required for each x, y and z = height of the
            // tree = level_ - 1
            // therefore, height of more than 21 is not supported
            unsigned int height = maxlevel - 1;

            int key = 1;
            long x_64 = (long) x << (64 - height);
            long y_64 = (long) y << (64 - height);
            long z_64 = (long) z << (64 - height);
            long mask = 1;
            mask = mask << 63; // leftmost bit is 1, rest 0

            for (unsigned int i = 0; i < height; ++i) {
                key = key << 1;
                if (x_64 & mask)
                    key = key | 1;
                x_64 = x_64 << 1;

                key = key << 1;
                if (y_64 & mask)
                    key = key | 1;
                y_64 = y_64 << 1;

                key = key << 1;
                if (z_64 & mask)
                    key = key | 1;
                z_64 = z_64 << 1;
            } // for

            return key;
        }

        long getCellId(OctreePoint &p) {
            //return the cell id for a point;
            //r stands for range
            double x = p.x - globalMinX;
            double y = p.y - globalMinY;
            double z = p.z - globalMinZ;
            double rx = globalMaxX - globalMinX;
            double ry = globalMaxY - globalMinY;
            double rz = globalMaxZ - globalMinZ;
            double domain = (rx > ry && rx > rz) ? rx : ((ry > rz) ? ry : rz);

            unsigned int numCells = 1 << (maxlevel - 1);
            double cellSize = domain / numCells;

            int xKey = (x / cellSize);
            int yKey = (y / cellSize);
            int zKey = (z / cellSize);

            return computeKey(xKey, yKey, zKey);
        }

        void setUpPointIds() {
            setUpGlobalMinMax();
            for (unsigned i = 0; i < this->localBuffer.size(); i++) {
                OctreePoint &p = this->localBuffer[i];
                p.cellId = getCellId(p);
            }
        }

        std::vector<long> getLocalSample() {
            //Sort the points based on their cell ids
            std::sort((this->localBuffer).begin(),(this->localBuffer).end(), cmpPoint);

            //Store the pivot locations for sample sort
            std::vector<long> m_cbuffer;
            //sample local data points
            double step = 1.0 * this->localBuffer.size() / numProc;
            int start = numProc==1? 0: round((step - 1) * (1.0 * procRank / (numProc - 1)));
            for (int i = 0; i < numProc; i++) {
                int index = start + i * step;
                long id = (this->localBuffer[index]).cellId;
                m_cbuffer.push_back(id);
            }
            return m_cbuffer;
        }

        int removeDuplicatesAndSort(long* nodeIds, int size) {
            std::set<int> s(nodeIds, nodeIds + size);
            int i = 0;
            for(auto&& it:s) nodeIds[i++] = it;

            return s.size();
        }

        std::vector<long> performSampleSort(std::vector<long> m_cbuffer) {
            long* m_pivotbuffer; // qqiu0809: memory leak? forgot to delete m_pivotbuffer?

            int size=comm.gatherAll(&m_cbuffer[0],m_cbuffer.size(), m_pivotbuffer);

            size = removeDuplicatesAndSort(m_pivotbuffer, size);
            //Sample global pivots
            double step = 1.0 * size / numProc;
            m_cbuffer.clear();
            for (int i = 0; i < numProc; i++) {
                m_cbuffer.push_back(m_pivotbuffer[int(i * step)]);
            }
            return m_cbuffer;
        }

        std::vector<int> getLengthsArray(const std::vector<T> &data, const std::vector<long> &splitter) {
            std::vector<int> lengths(numProc, 0);
            size_t i = 0;
            for (size_t j = 0; j < splitter.size() - 1; j++) {
                long left = (j==0)?-1:splitter[j];
                long right = splitter[j + 1];
                while (data[i].cellId < right && data[i].cellId >= left) {
                    lengths[j]++;
                    i++;
                }
            }
            while(i<data.size()){
                lengths[numProc - 1]++;
                i++;
            }
            return lengths;
        }

        void distributePoints(std::vector<long> splitters) {
            //get the lengths each processors gets from the data array
            std::vector<int> processorBucketSizes = getLengthsArray(this->localBuffer, splitters);

            //distribute the local buffer based on the lengths assigned for each processors
            comm.redistribute(this->localBuffer, processorBucketSizes);

            //sort the data (not needed perhaps)
            std::sort(this->localBuffer.begin(), this->localBuffer.end(), cmpPoint);

            //for (auto&& it : this->localBuffer)
            //   cout << procRank << ": retrieved node " << it.cellId << endl;
        }


    protected:

        //buffer array that stores private data
        std::vector<T> private_data;

        virtual void _postLoad(){
            comm.bcast(&maxlevel,1);
        }

        //sl15: required to be implemented by the implementer
        void _readPoints() {
            if (comm.isMaster()) {
                std::ifstream fs(this->filename.c_str());
                std::string line;
                bool datastart = false;

                while (std::getline(fs, line)) {
                    std::istringstream iss(line);
                    std::string params;
                    if (!datastart) {
                        iss >> params;
                        if (params == "NDIM" && !ndim)
                            iss >> ndim;
                        if (params == "MAX_LEVEL" && !maxlevel)
                            iss >> maxlevel;
                        if (params == "DATA_TABLE") {
                            datastart = true;
                            continue;
                        }
                    } else {
                        this->add(T(iss));
                    }
                }
            }
        }

        void _sort() {
            //set labels for each point (node ids)
            setUpPointIds();

            //get a sample of node ids from the local processor
            std::vector<long> localSamples = getLocalSample();

            //perform sample sort on the local node set and get the global pivots
            std::vector<long> globalPivots = performSampleSort(localSamples);

            //redistribute the points based on the global pivots
            distributePoints(globalPivots);
        }

        void _clear_localbuffer() {
            this->localBuffer.clear();
        }

        void _borrow_first_node(std::vector<long>& boundry_array){
            long *boundry_array_buffer; 

            comm.gatherAll(&localArr[0].id, 1, boundry_array_buffer);
            for (int i = 0; i < numProc; i++) {
                boundry_array.push_back(boundry_array_buffer[i]);
            }

            if(procRank != numProc - 1 ){
                long cellId = boundry_array[(procRank + 1) % numProc];
                //create empty node at the end of localArr with borrowed id;
                localArr.push_back(Node<T>(cellId));
                nodeTable[cellId] =  localArr.size() - 1;
            }else{
                //qqiu0809: Does the last processor need to borrow from the first one, or 
                //just ignore?
            }

            delete [] boundry_array_buffer;
        }

        void _flush_buffer() {
            cout << "DEBUG " <<procRank << ":  " << "localBuffer size: " << this->localBuffer.size() <<endl;
            //insert points to LocalArr
            //use nodeTable to do book keeping
            for (auto it : this->localBuffer) {
                long cellId = getCellId(it);
                auto itt = nodeTable.find(cellId);
                if (itt != nodeTable.end()){ 
                    //found existing
                    localArr[(*itt).second]._insert(it);
                }
                else {
                    //new node
                    localArr.push_back(Node<T>(it, cellId)); 
                    nodeTable[cellId] = localArr.size() - 1;
                }
            }
            comm.barrier();
            cout << "DEBUG " <<procRank << ":  " << "localArr size: " << this->localArr.size() <<endl;
            cout << "DEBUG " <<procRank << ":  " << "nodeTable size: " << this->nodeTable.size() <<endl;
            _borrow_first_node(boundry_array);

            cout << "DEBUG " <<procRank << ":  " << "localArr size after borrow: " << this->localArr.size() <<endl;
            cout << "DEBUG " <<procRank << ":  " << "nodeTable size after borrow: " << this->nodeTable.size() <<endl;
            _clear_localbuffer();
        }

        void _assemble() {
            std::set<int> aux;
            int old_last = -1;
            int new_last = this->localArr.size()-1;

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
                new_last = this->localArr.size()-1;
            }

            this->localArr.back().parent = -1;
            for(auto&& it : this->localArr){
                if(it.childset.empty())
                    it.childset.insert(std::make_pair(-1,-1));
            }

            cout << "DEBUG after assemble " <<procRank << ":  " << "localArr size: " << this->localArr.size() <<endl;
            cout << "DEBUG after assemble " <<procRank << ":  " << "nodeTable size: " << this->nodeTable.size() <<endl;

            _post_order_walk(&Tree::_add_to_tree);
        }

        //additional implementations by the implementer
        inline void _add_to_tree(int a) {
            this->localStruct.push_back(a);
        }

        void _post_order_walk(void (Tree::*fun)(int)) {
            std::stack<int> aux;
            auto track = new std::vector<NodeOrderSet::iterator>;
            std::vector<Node<T> >& localArr = this->localArr;

            for (auto&& it : localArr)
                track->push_back(it.childset.begin());

            int last_visited = -1;
            int current = localArr.size() - 1;
            int peek = 0;
            while (!aux.empty() || current != -1) {
                if (current != -1) {
                    aux.push(current);
                    current = (*track)[current]->first;
                } else {
                    peek = aux.top();
                    ++(*track)[peek];
                    if ((*track)[peek] != localArr[peek].childset.end()
                            && last_visited != ((*track)[peek])->first) {
                        current = (*track)[peek]->first;
                    } else {
                        (this->*fun)(peek);
                        last_visited = aux.top();
                        aux.pop();
                    }
                }
            }
            delete track;
        }

        std::string getLocalTree() const {
            std::string result("");
            for(auto it = this->localStruct.begin(); it != this->localStruct.end(); it++){
                auto bset = std::bitset<7>(localArr[*it].id);
                result = result + bset.to_string() + "*";
            }
            if(!result.empty()) result.pop_back();
            return result;
        }

        std::string getLinearTree() const {
            std::string result("");
            for(auto it = localArr.rbegin(); it != localArr.rend(); it++){
                auto curr_bset = std::bitset<7>((*it).id);
                result = result + curr_bset.to_string() + "(" + std::to_string((*it).getCount()) + ")" + "[";
                for(auto itt = (*it).childset.begin(); itt != (*it).childset.end(); itt++){
                    if((*itt).first != -1){
                        auto child_bset = std::bitset<7>(localArr[(*itt).first].id);
                        result = result + child_bset.to_string() + ",";
                    }
                }
                if(result.back() == ',') result.pop_back();
                result = result + "];";
            }
            if(!result.empty()) result.pop_back();
            return result;
        }

        inline bool isEmpty() const { return this->localBuffer.empty() && localArr.empty() && this->localStruct.empty(); }


        void free(){
            //TODO free allocated memory
        }

        inline void _endState() {
            this->msgBuffer.flush();
        }

        void _load(std::string filename) {
            this->filename = filename;
            if (this->msgBuffer.msgBufferLayer.isMaster()) {
                _readPoints();
                _endState();
                //this->msgBuffer.sendControlMessage(APC_SET_STATE, NAS_LOAD_COMPLETE);
                this->msgBuffer.msgBufferLayer.barrier();
            } else {
                this->msgBuffer.msgBufferLayer.barrier();
                //_setState(NAS_LOADING);
                this->_pumpNetwork();
            }
            _postLoad();
            _sort();
        }

    public:
        Tree() :Messager<T>() {
            comm=this->msgBuffer.msgBufferLayer;
        }

        void printData() const {
            std::cout << " ";
            for (auto&& it : private_data)
                std::cout << it << " ";
            std::cout << std::endl;
        }

        void build(std::string filename) {
            _load(filename);
            _flush_buffer();
            _assemble();
        }

        void print(std::ostream& stream) const {
            std::string linear_str = getLinearTree();
            std::string tree_str = getLocalTree();
            stream << linear_str << std::endl;
            stream << tree_str << std::endl;
        }
};
