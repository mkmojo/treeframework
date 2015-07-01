#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <queue>
#include <stack>
#include <set>
#include <math.h>

#include "../comm/Messager.hpp"

struct OctreePoint{
	double x, y, z;
	long cellId;
	OctreePoint():x(0.0),y(0.0),z(0.0),cellId(0){}
};

bool cmpPoint(const OctreePoint &p1, const OctreePoint &p2)
{
    return  p1.cellId < p2.cellId;
}

template<typename T> class Tree : public Messager<T> {
private:
   int ndim, maxlevel;
   double localMinX, localMaxX;
   double localMinY, localMaxY;
   double localMinZ, localMaxZ;
   double globalMinX, globalMaxX;
   double globalMinY, globalMaxY;
   double globalMinZ, globalMaxZ;
   std::vector<long> m_cbuffer;
   std::vector<long> m_pivotbuffer;
   //all start point
   std::vector<int> m_allstarts;
   //all lengths
   std::vector<int> m_alllengths;
   //local results of sort
   std::vector<T> m_sort_buffer;

   //Find local min and max for each dimension
    //Find global min and max through MPI ALLreduce
    //distribute the result
    void setUpGlobalMinMax()
    {
        //TODO: what do we do with a processor that does not
        //have any data?
        assert(Messager<T>::localBuffer.size() > 0);
        OctreePoint cntPoint = (OctreePoint) Messager<T>::localBuffer[0];
        double minX(cntPoint.x), maxX(cntPoint.x);
        double minY(cntPoint.y), maxY(cntPoint.y);
        double minZ(cntPoint.z), maxZ(cntPoint.z);

        for(unsigned i=1;i<Messager<T>::localBuffer.size();i++){
        	OctreePoint cntPoint = Messager<T>::localBuffer[i];
            //For current smallest cord
            if(cntPoint.x < minX){
                minX = cntPoint.x;
            }
            if(cntPoint.y < minY){
                minY = cntPoint.y;
            }
            if(cntPoint.z < minZ){
                minZ = cntPoint.z;
            }

            //For current lareges cord
            if(cntPoint.x > maxX){
                maxX = cntPoint.x;
            }
            if(cntPoint.y > maxY){
                maxY = cntPoint.y;
            }
            if(cntPoint.z > maxZ){
                maxZ = cntPoint.z;
            }
        }

        localMinX = minX;
        localMinY = minY;
        localMinZ = minZ;
        localMaxX = maxX;
        localMaxY = maxY;
        localMaxZ = maxZ;

        //set global dimension
        globalMinX = Messager<T>::msgBuffer.msgBufferLayer.reduce(minX, MIN);
        globalMinY = Messager<T>::msgBuffer.msgBufferLayer.reduce(minY, MIN);
        globalMinZ = Messager<T>::msgBuffer.msgBufferLayer.reduce(minZ, MIN);
        //set global dimension
        globalMaxX = Messager<T>::msgBuffer.msgBufferLayer.reduce(maxX, MAX);
        globalMaxY = Messager<T>::msgBuffer.msgBufferLayer.reduce(maxY, MAX);
        globalMaxZ = Messager<T>::msgBuffer.msgBufferLayer.reduce(maxZ, MAX);
    }
    long computeKey( unsigned int x, unsigned int y, unsigned z)
	{
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

		for(unsigned int i = 0; i < height; ++i) {
			key = key << 1;
			if(x_64 & mask) key = key | 1;
			x_64 = x_64 << 1;

			key = key << 1;
			if(y_64 & mask) key = key | 1;
			y_64 = y_64 << 1;

			key = key << 1;
			if(z_64 & mask) key = key | 1;
			z_64 = z_64 << 1;
		} // for

		return key;
	}

    long getCellId(OctreePoint &p)
    {
        //return the cell id for a point;
        //r stands for range
        double x = p.x - localMinX;
        double y = p.y - localMinY;
        double z = p.z - localMinZ;
        double rx = localMaxX - localMinX;
        double ry = localMaxY - localMinY;
        double rz = localMaxZ - localMinZ;

        double domain = (rx > ry && rx > rz) ?
            rx :
            ((ry > rz) ? ry : rz);

        unsigned int numCells = 1 << (maxlevel - 1);
        double cellSize = domain / numCells;
        int xKey = (unsigned int) (x / cellSize);
        int yKey = (unsigned int) (y / cellSize);
        int zKey = (unsigned int) (z / cellSize);

        return computeKey(xKey, yKey, zKey);
    }

    void setUpPointIds()
     {
         for(unsigned i=0; i < Messager<T>::localBuffer.size(); i++) {
         	OctreePoint p=(OctreePoint)Messager<T>::localBuffer[i];
         	p.cellId=getCellId(p);
         }
     }

//     bool wayToSort(int i, int j) { return i > j; }
     void sortLocalPoints()
     {
         long numTotal = 0;
         numTotal = Messager<T>::msgBuffer.msgBufferLayer.reduce((long)Messager<T>::localBuffer.size(),SUM);
         if(Messager<T>::msgBuffer.msgBufferLayer.isMaster())
             std::cout<< "DEBUG: " << numTotal<<" int total."  <<std::endl;

         //TODO: extend this to user defined data type
         //So customized iterator/cmp function etc.
//         std::vector<int> intVec = {56, 32, -43, 23, 12, 93, 132, -154};
//         std::sort(intVec.begin(), intVec.end(), wayToSort);
         std::sort((Messager<T>::localBuffer).begin(), (Messager<T>::localBuffer).end(), cmpPoint);
     }

     void getLocalSample()
     {
         //sample local data points

     	double step = 1.0*Messager<T>::localBuffer.size()/numProc;
     	int start = round((step-1) * (1.0 * procRank / (numProc-1)));
     //	cout<<"step="<<step<<" data size = "<<m_data.size()<<" start="<<start<<endl;
         for(unsigned int i=0;i<numProc; i++)
         {
     		int index = int(start + i * step);
             long id = ((OctreePoint)Messager<T>::localBuffer[index]).cellId;
             m_cbuffer.push_back(id);
         }
     }
     void removeDuplicates(std::vector<long>& nodeIds){
     	std::set<int> s(nodeIds.begin(), nodeIds.end());
     	nodeIds.assign(s.begin(), s.end());
     }

     void setGlobalPivot()
     {
         unsigned int numGlobalSample= numProc * numProc;
         m_pivotbuffer.resize(numGlobalSample);
         if(Messager<T>::msgBuffer.msgBufferLayer.isMaster()){
        	 Messager<T>::msgBuffer.msgBufferLayer.gather(&m_cbuffer[0], m_cbuffer.size(),
                     &m_pivotbuffer[0], numProc);
             removeDuplicates(m_pivotbuffer);
             std::sort(m_pivotbuffer.begin(), m_pivotbuffer.end());
         }
         else{
        	 Messager<T>::msgBuffer.msgBufferLayer.receiveGather(&m_cbuffer[0], m_cbuffer.size(),
                     &m_pivotbuffer[0], numProc);
         }

         //Sample global pivots
         if(Messager<T>::msgBuffer.msgBufferLayer.isMaster()){
         	double step = 1.0*m_pivotbuffer.size()/numProc;
             for(int i=0;i<numProc; i++) {
                 long id = m_pivotbuffer[int(i*step)];
                 m_cbuffer[i] = id;
             }
         }

         //Bcast pivots to all procs
         if(Messager<T>::msgBuffer.msgBufferLayer.isMaster()) {
        	 Messager<T>::msgBuffer.msgBufferLayer.bcast(&m_cbuffer[0], numProc);
             m_cbuffer.resize(numProc);
         }
         else{
        	 Messager<T>::msgBuffer.msgBufferLayer.receiveBcast(&m_cbuffer[0], numProc);
             m_cbuffer.resize(numProc);
         }
     }

protected:
    //buffer array that stores private data
    std::vector<T> private_data;

    //sl15: required to be implemented by the implementer
    void _load(){
		if (Messager<T>::msgBuffer.msgBufferLayer.isMaster()) {
			std::ifstream fs(Messager<T>::filename.c_str());
			std::string line;
			bool datastart = false;

			while(std::getline(fs, line)){
				std::istringstream iss(line);
				std::string params;
				if(!datastart){
					iss >> params;
					if(params == "NDIM" && !ndim) iss >> ndim;
					if(params == "MAX_LEVEL" && !maxlevel) iss >> maxlevel;
					if(params == "DATA_TABLE"){
						datastart = true;
						continue;
					}
				}else{
					Messager<T>::add(T(iss));
				}
			}
//			if(verbosity == Verbose) std::cout << "preload done! " << std::endl;
		}
    }
    
    void _sort(){
    	setUpGlobalMinMax();
		setUpPointIds();
		sortLocalPoints();
		getLocalSample();
		setGlobalPivot();
//		distributePoints();
    }

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

    //sl15: this method needs to be overriden by the subclasses
public:
    Tree() : Messager<T>() { }
    
    void printData() const {
        std::cout << " ";
        for(auto&& it : private_data) std::cout << it << " ";
        std::cout << std::endl;
    }
};
