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

//		// communication buffer cannot be of type tree_node due to presence of vectors!!!!
//		serial_tree_node* send_buff = new serial_tree_node[num_oo_nodes];
//
//		if(send_count == NULL || ptr == NULL || send_buff == NULL) {
//			std::cerr << "Error in memory allocation (globalize_tree())." << std::endl;
//			return false;
//		} // if
//
//		int* recv_count;
//		serial_tree_node* recv_buff;
//
//		// initialize counts
//		for(int i = 0; i < mpi_data_.size(); ++i) {
//			send_count[i] = 0;
//			ptr[i] = 0;
//		} // for
//		// compute send counts for each processor
//		if(num_oo_nodes != 0) {
//			for(int i = total_nodes - 1; i >= total_nodes - num_oo_nodes; -- i) {
//				int j = mpi_data_.size() - 1;
//				while( !is_contained(boundary_array[j].cell_key_,
//							node_list_[i].small_cell()) ) {
//					-- j;
//				} // while
//				++ send_count[j];
//			} // for
//		} // if
//		int total_send_count = send_count[0];
//		for(int i = 1; i < mpi_data_.size(); ++i) {
//			ptr[i] = total_send_count;
//			total_send_count += send_count[i];
//		} // for
//
//		int* proc_array = new int[num_oo_nodes];
//			// to store the proc id where oo nodes are sent
//		if(proc_array == NULL) {
//			std::cerr << "Error in memory allocation (globalize_tree())." << std::endl;
//			return false;
//		} // if
//
//		// construct the send buffer
//		if(num_oo_nodes != 0) {
//			for(int i = total_nodes - 1; i >= total_nodes - num_oo_nodes; -- i) {
//				// find which processor this node should go to
//				int j = mpi_data_.size() - 1;
//				while( !is_contained(boundary_array[j].cell_key_,
//							node_list_[i].small_cell()) )
//					-- j;
//
//				// Count the number of children of this OO node that are also OO
//				// The structure member num_children_ of a node indicates the number
//				// of children local to the processor. That is why count must be subtracted
//				// from node_list_[i].num_children_ since those OO child nodes will
//				// no longer be local to the processor. This decrement is accounted
//				// for at a later stage by an equal number of increments (when the OO
//				// children are looking for their parent nodes.)
//				int count = 0;
//				int k = 0;
//				std::pair<children_iter, children_iter> children = node_list_[i].children();
//				for(children_iter iter = children.first; iter != children.second;
//						++ iter, ++ k) {
//					if((*iter).index_ >= total_nodes - num_oo_nodes)
//						++ count;
//					else // set the new parent processor location
//						node_list_[(*iter).index_].set_parent_proc(j);
//				} // for
//				node_list_[i].remove_last_children(count);
//				pack_node(send_buff[ptr[j]], node_list_[i]);
//				node_list_.pop_back();
//
//				// proc_array stores the proc id where OO is sent
//				proc_array[ptr[j]] = j;
//				++ ptr[j];
//			} // for
//		} // if
//
//		// All processors now exchange the respective OO nodes.
//		int recv_size = 0;
//		twlib::all_to_all(send_count, send_buff, recv_count, recv_buff,
//				recv_size, mpi_data_.comm());	// recv_size is total count
//
//		// make a note of all local nodes whose parent index needs to be updated later
//		std::vector<std::pair<int, int> > update_indices;
//		for(int i = 0; i < num_oo_nodes; ++ i) {
//			for(int j = 0; j < send_buff[i].num_children_; ++ j) {
//				int index = send_buff[i].children_[j].index_;
//				if(index < total_nodes - num_oo_nodes)
//					update_indices.push_back(std::make_pair(index, i));
//			} // for
//		} // for
//
//		// Merge the received oo nodes to local nodes and
//		// let the other 'sending' procs know about the new location
//
//		new_node_count_ = 0;
//		int* notify_array = new int[recv_size];
//		bool* present_array = new bool[recv_size];
//		if(notify_array == NULL || present_array == NULL) {
//			std::cerr << "Error in memory allocation (globalize_tree())." << std::endl;
//			return false;
//		} // if
//		std::vector<tree_node> temp_recv_buff;
//
//		// the node list is scanned as many times as recv_size - could be improved ???
//		// (may be use binary search first in the local nodes, and then scan the temp_recv_buff)
//		// check if a received node is already present in local node list
//		// if it is, merge its children array, otherwise, store the new node in temp_recv_buff
//		for(int i = 0; i < recv_size; ++ i) {
//			present_array[i] = false;
//			tree_node temp_node;
//			unpack_node(recv_buff[i], temp_node);
//
//			for(int j = 0; j < total_nodes - num_oo_nodes + new_node_count_; ++ j) {
//				if(node_list_[j].small_cell() == temp_node.small_cell()) {
//					// for every child in temp_node, add it to node_list_[j]
//					std::pair<children_iter, children_iter> children = temp_node.children();
//					for(children_iter iter = children.first; iter != children.second; ++ iter)
//						node_list_[j].add_child(*iter);
//					present_array[i] = true;
//
//					// need to check and add the child to node in temp_recv_buff
//					for(int k = 0; k < new_node_count_; ++ k) {
//						if(temp_recv_buff[k].small_cell() == temp_node.small_cell()) {
//							std::pair<children_iter, children_iter> children =
//									temp_node.children();
//							for(children_iter iter = children.first;
//									iter != children.second; ++ iter)
//								temp_recv_buff[k].add_child(*iter);
//						} // if
//					} // for
//
//					break;
//				} // if-else
//			} // while
//
//			// if it is not present, it is unpacked into temp_recv_buff
//			if(!present_array[i]) {
//				temp_recv_buff.push_back(temp_node);
//
//				// also temporarily insert it at the end of node_list_ in order to
//				// increase node_list_ and take care of duplicates
//				node_list_.push_back(temp_node);
//
//				++ new_node_count_;
//			} // if
//		} // for
//
//		// sort the new received nodes in temp_recv_buff
//		if(new_node_count_ > 1) postorder_sort(0, new_node_count_ - 1, temp_recv_buff);
//
//		// remove the last new_node_count_ nodes from node_list_
//		for(int i = 0; i < new_node_count_; ++ i) node_list_.pop_back();
//
//		// find the 'supposed to be' positions of these new nodes
//		// (again, may be improve by using binary search ??? )
//		std::vector<int> positions;
//		for(int i = 0; i < new_node_count_; ++ i) {
//			int j = 0;
//			while(j < node_list_.size()) {
//				if(node_list_[j] < temp_recv_buff[i]) ++ j;
//				else break;
//			} // while
//			positions.push_back(j);
//		} // for
//
//		// accordingly increase all the local indices for each position
//		for(int i = new_node_count_ - 1; i >= 0; -- i) {
//			for(int j = 0; j < node_list_.size(); ++ j) {
//				index_type parent = node_list_[j].parent();
//				if(parent.proc_id_ == mpi_data_.rank() && parent.index_ >= positions[i])
//					node_list_[j].set_parent_index(parent.index_ + 1);
//				// do for children also
//				std::pair<children_iter, children_iter> children = node_list_[j].children();
//				int k = 0;
//				for(children_iter iter = children.first;
//						iter != children.second; ++ iter, ++ k) {
//					if((*iter).proc_id_ == mpi_data_.rank() &&
//							(*iter).index_ >= positions[i])
//						node_list_[j].set_child_index(k, (*iter).index_ + 1);
//				} // for
//			} // for
//			for(int j = 0; j < total_send_count; ++ j)
//				for(int k = 0; k < send_buff[j].num_children_; ++ k)
//					if(send_buff[j].children_[k].proc_id_ == mpi_data_.rank() &&
//						send_buff[j].children_[k].index_ >= positions[i])
//							++ send_buff[j].children_[k].index_;
//			for(int j = 0; j < update_indices.size(); ++ j)
//				if(update_indices[j].first >= positions[i])
//					++ update_indices[j].first;
//
//			positions[i] += i;
//		} // for
//
//		// and then insert the un-present received nodes into node_list_
//		for(int i = 0; i < new_node_count_; ++ i) {
//			iterator pos = node_list_.begin() + positions[i];
//			node_list_.insert(pos, temp_recv_buff[i]);
//		} // for
//
//		// and construct the new_node_index_ array
//		new_node_index_.clear();
//		for(int i = 0; i < new_node_count_; ++ i)
//			new_node_index_.push_back(positions[i]);
//
//		// construct the notify_array for the positions of the received nodes
//		for(int i = 0; i < recv_size; ++ i) {
//			tree_node temp_node;
//			unpack_node(recv_buff[i], temp_node);
//			for(int j = 0; j < node_list_.size(); ++ j)
//				if(node_list_[j].small_cell() == temp_node.small_cell())
//					notify_array[i] = j;
//		} // for
//
//		// communicate back the location of the newly added nodes
//		int* to_be_discarded;
//		int* recv_back;
//		twlib::all_to_all(recv_count, notify_array, to_be_discarded, recv_back,
//				recv_size, mpi_data_.comm());
//
//		total_nodes = node_list_.size(); // total number of nodes
//
//		// update parent index info using recv_back
//		for(int i = 0; i < update_indices.size(); ++ i)
//			node_list_[update_indices[i].first].set_parent_index(
//												recv_back[update_indices[i].second]);
//
//		// update the parent and child information of the each new node
//		for(int i = 0; i < new_node_count_; ++ i) {
//			int index = new_node_index_[i];
//			int j = index - 1;
//
//			// the previous node is its child
//
//			node_list_[j].set_parent(make_index(mpi_data_.rank(), index));
//
//			// compute set the large cell information
//			uint64_t temp_large_cell = compute_large_cell(
//					node_list_[index].small_cell(), node_list_[j].small_cell());
//			node_list_[j].set_large_cell(temp_large_cell);
//
//			node_list_[index].add_child(make_index(mpi_data_.rank(), j));
//
//			if(index != total_nodes - 1) {
//				uint64_t temp_small_cell = compute_small_cell(
//						node_list_[index].small_cell(),	node_list_[index + 1].small_cell());
//				j = index + 1; // the parent can only be on the right side
//				while(j < total_nodes) {
//					if(node_list_[j].small_cell() == temp_small_cell) {
//						node_list_[index].set_parent(make_index(mpi_data_.rank(), j));
//						temp_large_cell = compute_large_cell(node_list_[j].small_cell(),
//								node_list_[index].small_cell());
//						node_list_[index].set_large_cell(temp_large_cell);
//						break;
//					} else ++ j;
//				} // while
//				if(j == total_nodes) {
//					int k = 0;
//					while(k < num_oo_nodes) {
//						if(send_buff[k].ttd_.small_cell_ == temp_small_cell) {
//							node_list_[index].set_parent(
//									make_index(proc_array[k], recv_back[k]));
//							temp_large_cell = compute_large_cell(
//									send_buff[k].ttd_.small_cell_,
//									node_list_[index].small_cell());
//							node_list_[index].set_large_cell(temp_large_cell);
//							break;
//						} else ++ k;
//					} // while
//				} // if
//			} else if(mpi_data_.rank() != mpi_data_.size() - 1) {
//				uint64_t temp_small_cell = compute_small_cell(
//						node_list_[index].small_cell(),
//						boundary_array[mpi_data_.rank() + 1].cell_key_);
//				int k = 0;
//				while(k < num_oo_nodes) {
//					if(send_buff[k].ttd_.small_cell_ == temp_small_cell) {
//						node_list_[index].set_parent(
//								make_index(proc_array[k], recv_back[k]));
//						temp_large_cell = compute_large_cell(
//								send_buff[k].ttd_.small_cell_,
//								node_list_[index].small_cell());
//						node_list_[index].set_large_cell(temp_large_cell);
//						break;
//					} else ++ k;
//				} // while
//			} else {
//				node_list_[index].set_parent(make_index(mpi_data_.rank(), -1));
//			} // if-else
//		} // for
//
//		// rectify the index information for all remote children:
//
//		// scan the local nodes, and collect info from all those nodes whose parent is remote
//		// the info collected is the parent index,
//		// and the node's index
//
//		// initialize send counts
//		for(int i = 0; i < mpi_data_.size(); ++i) {
//			send_count[i] = 0;
//			ptr[i] = 0;
//		} // for
//		// compute send count
//		for(int i = 0; i < node_list_.size(); ++ i) {
//			index_type parent = node_list_[i].parent();
//			if(parent.proc_id_ != mpi_data_.rank())	++ send_count[parent.proc_id_];
//		} // for
//		total_send_count = send_count[0];
//		for(int i = 1; i < mpi_data_.size(); ++ i) {
//			ptr[i] = total_send_count;
//			total_send_count += send_count[i];
//		} // for
//		// construct the send_child_buff
//		child_data* send_child_buff = new child_data[total_send_count];
//		if(send_child_buff == NULL) {
//			std::cerr << "Error in memory allocation (globalize_tree())." << std::endl;
//			return false;
//		} // if
//		for(int i = 0; i < node_list_.size(); ++ i) {
//			index_type parent = node_list_[i].parent();
//			if(parent.proc_id_ != mpi_data_.rank() && parent.index_ != -1) {
//				send_child_buff[ptr[parent.proc_id_]].parent_index_ = parent.index_;
//				send_child_buff[ptr[parent.proc_id_]].proc_id_ = mpi_data_.rank();
//				send_child_buff[ptr[parent.proc_id_]].index_ = i;
//				++ ptr[parent.proc_id_];
//			} // if
//		} // for
//
//		recv_size = 0;
//		int* recv_child_count;
//		child_data* recv_child_buff;
//		twlib::all_to_all(send_count, send_child_buff, recv_child_count, recv_child_buff,
//				recv_size, mpi_data_.comm());	// recv_size is total count
//
//		delete[] present_array;
//		delete[] recv_buff;
//		delete[] recv_count;
//		delete[] to_be_discarded;
//		delete[] recv_back;
//		delete[] send_count;
//		delete[] ptr;
//		delete[] send_buff;
//		delete[] proc_array;
//		delete[] notify_array;
//
//		// put the received child node info into multimap
//		// key = cell, value = <processor, index>
//		typedef std::multimap<const int, std::pair<int, int>, ltindex> child_map;
//		typedef typename std::multimap<const int, std::pair<int, int>, ltindex>::iterator
//			child_map_iter;
//
//		child_map children_map;
//		for(int i = 0; i < recv_size; ++ i) {
//			children_map.insert(std::pair<const int, std::pair<int, int> >(
//					recv_child_buff[i].parent_index_,
//					std::make_pair(recv_child_buff[i].proc_id_, recv_child_buff[i].index_)));
//		} // for
//
//		delete[] recv_child_buff;
//		delete[] recv_child_count;
//		delete[] send_child_buff;
//
//		// for each local node, search for the received cell in it and
//		// do the needful (update children)
//		int i = 0; // to scan through node_list_ only once
//		for(child_map_iter iter = children_map.begin(); iter != children_map.end(); ) {
//			// search for (*iter).first in node_list_
//			int index = (*iter).first;
//			node_list_[index].remove_remote_children(mpi_data_.rank());
//
//			// insert all the new children
//			while((*iter).first == index && iter != children_map.end()) {
//				node_list_[index].add_child(make_index((*iter).second.first,
//							(*iter).second.second));
//				++ iter;
//			} // while
//		} // for
//
//		// do the same for local nodes
//		child_map local_children;
//		for(int i = 0; i < node_list_.size(); ++ i) {
//			index_type parent = node_list_[i].parent();
//			if(parent.proc_id_ == mpi_data_.rank() && parent.index_ != -1) {
//				local_children.insert(std::pair<const int, std::pair<int, int> >(
//					parent.index_,
//					std::make_pair(mpi_data_.rank(), i)));
//			} // if
//		} // for
//
//		for(child_map_iter iter = local_children.begin(); iter != local_children.end(); ) {
//			int index = (*iter).first;
//			node_list_[index].remove_local_children(mpi_data_.rank());
//
//			// insert all the new children
//			while((*iter).first == index && iter != local_children.end()) {
//				node_list_[index].add_child(make_index((*iter).second.first,
//							(*iter).second.second));
//				++ iter;
//			} // while
//		} // for

		return true;
	} // globalize_tree()


protected:
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

	void _assemble() {
		vector< Node<T> > temp_nodes;
		temp_nodes.push_back(Node<T>(this->localBuffer[0].cellId));
		for(int i=1;i<this->localBuffer.size();i++){
			if (this->localBuffer[i-1].cellId!=this->localBuffer[i].cellId){
				temp_nodes.push_back(Node<T>(this->localBuffer[i].cellId));
			}
		}
		int n_unique=temp_nodes.size();
		NodeIndex nodeIndex = NodeIndex();
		nodeIndex.processorId=procRank;
		nodeIndex.index=-1;
		parentIndexMap[temp_nodes[0].getId()] = nodeIndex;
		temp_nodes[0].setParent(1);
		temp_nodes[0].setType(LEAF);

//		temp_nodes.resize(2*n_unique);

		int j = n_unique;

		// insert all leaves
		for(unsigned int i = 1; i < n_unique; ++i) {
			insert_leaf(temp_nodes[i], temp_nodes, i, j, n_unique);
			temp_nodes[i].setType(LEAF);
		}

		long* boundary_array;
		long cellId=this->localBuffer[0].cellId;
		comm.gatherAll(&cellId, 1, boundary_array);


		if(!comm.isLastProc()) {
			Node<T> borrowed_leaf=Node<T>(boundary_array[procRank + 1]);
			borrowed_leaf.setType(LEAF);
			insert_leaf(borrowed_leaf, temp_nodes, n_unique, j,	n_unique);
		} // if

		// construct the postordering of the local tree
		int total_nodes = j;
		int count = 0;		// number of nodes
		int local_root = get_root(temp_nodes);
		construct_postorder(temp_nodes, local_root, count);

		// put the datapoints into the leaves
		insert_data_points(total_nodes);

		comm.barrier();

		// identify 'out of order' nodes and put them on the correct processor
		// (last_small_cell is the small cell of the last leaf)
		long last_small_cell = temp_nodes[n_unique - 1].getId();
		globalize_tree(last_small_cell, n_unique, total_nodes,
				maxlevel, boundary_array);
		//
		//		// set the last node on last processor as root
		//		if(mpi_data_.rank() == mpi_data_.size() - 1)
		//			node_list_[node_list_.size() - 1].make_root();
		//
		//		// set the index for each node
		//		for(int i = 0; i < node_list_.size(); ++ i)
		//			node_list_[i].set_index(make_index(mpi_data_.rank(), i));
		//
		//		levels_ = octree_data.levels();
		//		// number of leaves = octree_data.n_unique() (if needed in future)
		//		domain_ = octree_data.domain();
		//
		//		delete[] boundary_array;
		//
		//		MPI_Barrier(mpi_data_.comm());
		//		double construct_time_e = MPI_Wtime();
		//
		//		int num_nodes = node_list_.size();
		//		int* total_num_nodes = new int[mpi_data_.size()];
		//		if(total_num_nodes == NULL) {
		//			std::cerr << "Error in memory allocation (total_num_nodes in construct_c_octree())."
		//				<< std::endl;
		//			return false;
		//		} // if
		//		MPI_Allgather(&num_nodes, 1, MPI_INT, total_num_nodes, 1, MPI_INT, mpi_data_.comm());
		//		if(mpi_data_.rank() == 0) {
		//			std::cout << "done: "
		//				<< (construct_time_e - construct_time_s) * 1000 << "ms." << std::endl;
		//			num_nodes = 0;
		//			for(int i = 0; i < mpi_data_.size(); ++i) num_nodes += total_num_nodes[i];
		//			std::cout << "Number of nodes: " << num_nodes;
		//			std::cout << ", Levels: " << levels_ << std::endl;
		//		} // if
		//
		//		delete[] total_num_nodes;
		//
		//		// Tree construction is complete,
		//		// now perform post processing to make the tree ready
		//
		//		double postprocess_time_s = MPI_Wtime();
		//
		//		// make the tree ready for accumulation computations:
		//		// (probably merge residual tree construction and contraction together ... )
		//		construct_residual_tree(mpi_data_);
		//		construct_contraction(mpi_data_);
		//		compute_levels(mpi_data_);
		//
		//		// Collect the leftmost and rightmost nodes cells from all processors.
		//		// This is useful in identifying the processor id where any given cell should reside.
		//		serial_tree_node *leftmost, *rightmost; // the local leftmost and rightmost nodes
		//		leftmost = new serial_tree_node;
		//		rightmost = new serial_tree_node;
		//		serial_tree_node *all_leftmost, *all_rightmost; // arrays for the results
		//		int recv_size = 0;
		//
		//		pack_node(leftmost[0], node_list_[0]);
		//		pack_node(rightmost[0], node_list_[node_list_.size() - 1]);
		//
		//		twlib::all_gather(leftmost, 1, all_leftmost, recv_size, mpi_data_.comm());
		//		twlib::all_gather(rightmost, 1, all_rightmost, recv_size, mpi_data_.comm());
		//
		//		tree_node temp_node;
		//		for(int i = 0; i < mpi_data_.size(); ++ i) {
		//			unpack_node(all_leftmost[i], temp_node);
		//			boundaries_.push_back(temp_node);
		//			unpack_node(all_rightmost[i], temp_node);
		//			boundaries_.push_back(temp_node);
		//		} // for
		//
		//		delete[] all_rightmost;
		//		delete[] all_leftmost;
		//		delete rightmost;
		//		delete leftmost;
		//
		//		// Make the tree ready for One-sided communications
		//		//
		//		// Serialize the node_list_ and store it in a linear array
		//		// Each node in this linear array will be of exactly the same size
		//		// (use the serial_tree_node). This is not the general case since this
		//		// will be specialized only for octrees. Think of a better way to do this
		//		// later ...
		//
		//		int size = node_list_.size() * sizeof(serial_tree_node);
		//		int ret = MPI_Alloc_mem(size, MPI_INFO_NULL, &serialized_node_list_);
		//		if(ret == MPI_ERR_NO_MEM) {
		//			std::cerr << "Error in allocating serial MPI memory! Aborting." << std::endl;
		//			return false;
		//		} // if
		//
		//		// serialize the tree
		//		serialize_tree(node_list_, serialized_node_list_);
		//
		//		// Create a window on this linear array
		//		MPI_Win_create(serialized_node_list_, size, sizeof(serial_tree_node),
		//				            MPI_INFO_NULL, mpi_data_.comm(), &node_list_win_);
		//
		//		MPI_Barrier(mpi_data_.comm());
		//		double postprocess_time_e = MPI_Wtime();
		//
		//		if(mpi_data_.rank() == 0) std::cout << "postprocessing done: "
		//			<< (postprocess_time_e - postprocess_time_s) * 1000 << "ms." << std::endl;
		//
		//		unsigned long int memory = jaz::mem_usage();
		//		if(mpi_data_.rank() == 0)
		//			std::cout << "Memory usage: " << (memory / 1024) << " KB" << std::endl;

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

	//sl15: this method needs to be overriden by the subclasses
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
};
