#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <queue>
#include <stack>
#include <set>
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
        map< int ,NodeIndex> parentIndexMap;


        std::vector<Node<T> > localArr;
        std::unordered_map<long, int> nodeTable;

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
            long* m_pivotbuffer;

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

            for (auto&& it : this->localBuffer)
                cout << procRank << ": retrieved node " << it.cellId << endl;
        }

        bool is_equal(double a, double b) {
            if(fabs(a - b) > 1e-9) return false;
            return true;
        } // is_equal()

        // returns the position of leftmost 1
        int log_2(uint64_t a) {
            int k = 0;

            while(a) {
                k++;
                a = a >> 1;
            }

            return k;
        }

        // function to find if a is contained in b (strictly)
        bool is_contained(long a, long b) {
            // get the positions of the leftmost 1
            int k = log_2(a);
            int l = log_2(b);

            if(a == 0 || b == 0) return false;

            if(b == 1) return true; // b is the whole domain

            if(l >= k) return false;    // cell a cannot have larger or equal sized key as b

            long temp1 = 0, temp2 = 0, temp3 = 0;
            temp1 = a << (64 - k);
            temp2 = b << (64 - l);
            temp3 = temp1 ^ temp2;

            int m = log_2(temp3);

            if(m > 64 - l) return false;

            return true;
        } // is_contained()

        // compute the large cell of a node given its small cell 'cell'
        // and its parent's small cell 'parent'.
        // The large cell is the cell obtained by first division of parent's
        // small cell, that contains the node's small cell.
        long compute_large_cell(long parent, long cell) {
            int k = log_2(parent);
            int l = log_2(cell);

            long large_cell = 0;

            large_cell = cell >> (l - k - 3);

            return large_cell;
        } // compute_large_cell()

        // Compute the smallest cell which contains the two given cells a and b
        long compute_small_cell(long a, long b) {

            if(is_contained(a, b)) return b;
            if(is_contained(b, a)) return a;

            int k = log_2(a);
            int l = log_2(b);

            long temp1 = 0, temp2 = 0, temp3 = 0, small_cell = 0;
            temp1 = a << (64 - k);
            temp2 = b << (64 - l);

            temp3 = temp1 ^ temp2;

            int m = log_2(temp3);
            int shift = 63 - ((63 - m) / 3) * 3;

            small_cell = temp1 >> shift;

            return small_cell;
        } // compute_small_cell()

        bool insert_leaf(Node<T>& curr_leaf, std::vector< Node<T> >& temp_nodes,
                const int i, int &j, const int m) {
            // i is the leaf to be inserted (curr_leaf == temp_nodes[i], when i < m)
            // j is the current number of nodes in the tree
            // m is the total number of leaf nodes

            // find the node k whose large cell contains the leaf i
            int k = i - 1;
            while( !is_contained(curr_leaf.getId(), temp_nodes[k].getParent()) )
                k = parentIndexMap[temp_nodes[k].getId()].index; // tree is local

            NodeIndex temp_index;

            // if leaf i is contained in small cell of k, insert i as child of k
            // else create a new internal node and make i and k the children of this node
            if( is_contained(curr_leaf.getId(), temp_nodes[k].getParent()) ) {
                // node k will become the parent of node i

                // set the large cell of i
                long large_cell = compute_large_cell(
                        temp_nodes[k].getId(),	curr_leaf.getId());
                curr_leaf.setParent(large_cell);

                // set k as parent of i
                parentIndexMap[curr_leaf.getId()]=NodeIndex(procRank,k);

                // set i as child of k (if i is not borrwoed leaf)
                if(i != m)
                    temp_nodes[k].addChild(NodeIndex(procRank,i));

            } else {
                // a new internal node j is created which is made the parent of nodes i and k

                long small_cell = compute_small_cell(
                        curr_leaf.getId(), temp_nodes[k].getId());

                // make the new node j
                temp_nodes.push_back(Node<T>(small_cell));
                temp_nodes[j].setParent(temp_nodes[k].getParent());

                // make parent of k as parent of j
                temp_index = parentIndexMap[temp_nodes[k].getId()];
                parentIndexMap[temp_nodes[j].getId()]=temp_index;
                temp_index.processorId=procRank;

                // make i and k the children of j
                temp_nodes[j].clearChildren();
                temp_nodes[j].addChild(NodeIndex(procRank, k));
                if(i != m)
                    temp_nodes[j].addChild(NodeIndex(procRank, i));

                // make j the child of k's parent
                if(temp_index.index != -1) {
                    // check why is k always the last child of its parent ???
                    temp_nodes[temp_index.index].updateLastChild(
                            NodeIndex(procRank, j));
                } // if

                // make j the parent of i and k
                long large_cell = compute_large_cell(
                        temp_nodes[j].getId(), temp_nodes[i].getId());
                curr_leaf.setParent(large_cell);
                parentIndexMap[curr_leaf.getId()]=NodeIndex(procRank, j);

                large_cell = compute_large_cell(
                        temp_nodes[j].getId(), temp_nodes[k].getId());
                temp_nodes[k].setParent(large_cell);
                parentIndexMap[temp_nodes[k].getId()]=NodeIndex(procRank, j);

                ++j; // increase the total number nodes

            } // if-else

            return true;
        } // insert_leaf()

        int get_root(const std::vector<Node<T> >& temp_nodes) {
            int root_index = 0;

            while(parentIndexMap[temp_nodes[root_index].getId()].index != -1){
                root_index = parentIndexMap[temp_nodes[root_index].getId()].index;
            }
            return root_index;
        } // get_root()

        bool construct_postorder(const std::vector<Node<T> >& temp_nodes,
                const int& root, int& count) {

            int num_children = temp_nodes[root].numChildren();
            if(num_children == 0) {
                Node<T> temp1 = temp_nodes[root];
                temp1.clearChildren();
                node_list.push_back(temp1);
                ++ count;
                return true;
            } // if

            std::vector<NodeIndex>* children=temp_nodes[root].getChildren();
            std::vector<int> children_index;
            for(int i=0;i<children->size();i++){
                construct_postorder(temp_nodes, children->at(i).index, count);
                children_index.push_back(count - 1);
            }

            Node<T> temp1 = temp_nodes[root];
            temp1.clearChildren();
            node_list.push_back(temp1);

            // insert temp_nodes[root] and set its parent

            // set the parent child relations
            for(int i=0;i<children_index.size();i++){
                //			children_index[i]
                parentIndexMap[node_list[children_index[i]].getId()]=NodeIndex(procRank, count);
                node_list[count].addChild(NodeIndex(procRank, children_index[i]));
            }

            ++count;

            return true;
        } // construct_postorder()

        std::vector<Node<T> > node_list;

        bool insert_data_points(const int& total_nodes) {
            for(int i = 0, j = 0; i < this->localBuffer.size(); ++ j) {
                if(node_list[j].getType()==LEAF) {
                    while(this->localBuffer[i].cellId == node_list[j].getId()) {
                        node_list[j]._insert(this->localBuffer[i]);
                        ++ i;
                        if(i >= this->localBuffer.size()) break;
                    } // while
                } // if
            } // for
            return true;
        } // insert_data_points()

        bool globalize_tree(const long last_small_cell, unsigned int m,
                int& total_nodes, unsigned int levels,
                const long* boundary_array) {

            //		typedef typename tree_node::const_children_iterator children_iter;

            // count the number of 'out of order' (oo) nodes (nodes to right of borrowed leaf)
            int num_oo_nodes = 0;
            if(!comm.isLastProc()) {
                int i = total_nodes - 1;
                while(node_list[i].getId() != last_small_cell) {
                    if(is_contained(boundary_array[procRank + 1],
                                node_list[i].getId())) {
                        ++ num_oo_nodes;
                        -- i;
                    } else {
                        break;
                    } // if-else
                } // while
            } // if

            // prepare the send buffers containing the oo nodes to be sent,
            // identify the processor id to where to send the oo nodes, and
            // perform the communications to send and receive the oo nodes

            int* send_count = new int[numProc];
            int* ptr = new int[numProc];

            return true;
        } // globalize_tree()


    protected:
        inline Node<T>& _local(int a) { return this->localArr[a]; }

        //buffer array that stores private data
        std::vector<T> private_data;

        virtual void _postLoad(){
            comm.bcast(&maxlevel,1);
        }

        //sl15: required to be implemented by the implementer
        void _load() {
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

        void _flush_buffer() {
            cout << "DEBUG " <<procRank << ":  " << "localBuffer size: " << this->localBuffer.size() <<endl;
            for (auto it : this->localBuffer) {
                long cellId = getCellId(it);
                auto itt = nodeTable.find(cellId);
                if (itt != nodeTable.end()){ 
                    localArr[(*itt).second]._insert(it);
                }
                else {
                    localArr.push_back(Node<T>(it, cellId)); 
                    nodeTable[cellId] = localArr.size() - 1;
                }
            }
            cout << "DEBUG " <<procRank << ":  " << "localArr size: " << this->localArr.size() <<endl;
            cout << "DEBUG " <<procRank << ":  " << "nodeTable size: " << this->nodeTable.size() <<endl;
            _clear_localbuffer();
        }



        void _assemble() {
            std::set<int> aux;
            int old_last = -1;
            int new_last = localArr.size()-1;

            while(new_last - old_last > 1){ 
                for(int i = old_last+1; i <= new_last; i++)
                    aux.insert(_local(i).id >> 3);
                old_last = localArr.size()-1;

                for(auto it : aux){
                    localArr.push_back(Node<T>(it));
                    for(int i = 0; i < localArr.size()-1; i++){
                        if((_local(i).id) >> 3 == it){
                            localArr.back().childset.insert(std::make_pair(i, _local(i).id));
                            _local(i).parent = localArr.size()-1;
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

            //_post_order_walk(&Tree::_add_to_tree);

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

        void free(){
            //TODO free allocated memory
        }

        inline void _endState() {
            this->msgBuffer.flush();
        }

        void runSimple(std::string filename) {
            this->filename = filename;
            if (this->msgBuffer.msgBufferLayer.isMaster()) {
                _load();
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
            runSimple(filename);
            _flush_buffer();
            _assemble();
        }
};
