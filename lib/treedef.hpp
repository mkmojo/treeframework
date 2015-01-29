#ifndef TREEDEF_H
#define TREEDEF_H 1
/** @file treedef.hpp
 *
 *  @brief This header file contains base classes and default template definitions to use the tree framework
 *
 *  @author Shule Li <shuleli@berkeley.edu> Qiyuan Qiu <qiuqiyuan@gmail.com> Saminda Wijeratne <>
 *
 *  @date 12-20-14
 *  
 *  @bug No known bugs.
 *
 *  @copyright Rochester Software License
 */

#include <string>
#include <set>
#include <list>
#include <memory>
#include <exception>
#include <vector>
#include <fstream>
#include <iostream>
//#include "MessageBuffer.h"

namespace treedef{
	class TreeNode;
	using NodePtr = std::shared_ptr<TreeNode>;
	using NodeList = std::vector<std::shared_ptr<TreeNode> >;
	using NodeSet = std::set<std::shared_ptr<TreeNode> >;
	NodeList* Generate(NodePtr);
	void Combine(double*, double*);
	void Evolve(double*, double*);
	void TreeCombine(NodePtr, NodePtr);
	//MessageBuffer m_comm;


	class TreeNode {
		typedef std::shared_ptr<U> VarPtr;
		typedef std::shared_ptr<V> ComPtr;

		public:
		VarPtr pVar; //< pointer to the input field of a tree node
		ComPtr pCom; //< pointer to the combined field of a tree node
		NodeSet *pGen, *pOwn; //< pointer to the generate set and owner set
		int dGen, dOwn;  //< generate and owner counter for the residual tree 
		NodePtr left, parent, sibling; //< pointer to the neighbors

		TreeNode(double*); //< constructor: a pointer to the initial input field value is passed in
		~TreeNode(); //< destructor
		void getGenSet(NodeList*);
	};

	/** @brief This function contructs a tree node. U: input data type, V: combined type
	 *  @param pVar_in pointer to the input data structure
	 */
	TreeNode::TreeNode(double* pVar_in):dGen(0),dOwn(0) {
		pVar = pVar_in; //< copy to shared pointer
		pGen = new NodeSet; //< initialize generate set
		pOwn = new NodeSet; //< initialize owner set
	}

	/** @brief This function contructs a generate set from a user defined generate list. U: input data type, V: combined type
	 *  @param list_in pointer to the input list
	 *  @return void
	 */
	void TreeNode::getGenSet(NodeList* list_in){
		if(list_in->empty()) return;
		//for(auto it = list_in.begin(); it != list_in.end(); it++){
		//    this->pGen->insert(*it); //< insert list nodes into the generate set
		//    (*it)->pOwn->insert(this); //< insert this into the list nodes' own set
		//    (*it)->dOwn++; //< increment the owner's counter
		//}
		//this->dGen = list_in.size(); //< set generate counter to be the length of the list

		list_in->clear(); //< clear the list
		delete(list_in);
	}

	/** @brief This function contructs a generate list for one node. It needs to be specialized by the user. GenID: type of generate, T: input data type, R: combined type
	 *  @param u pointer to the node being worked on
	 *  @return NodeList
	 */
	NodeList* Generate(NodePtr u) {
		//> default generate function for tree:
		auto pChild = u->left;
		auto tmplist = new NodeList;

		//> push all children into the list
		do{
			tmplist->push_back(pChild);
		} while(pChild.sibling != nullptr);
		return tmplist;
	}

	/** @brief This function combines input data type stored in v into u. It needs to be specialized by the user. ComID: type of combine, T: input data type, R: combined type
	 *  @param u pointer to the data structure storing the combined value
	 *  @param v pointer to the data structure storing the input value
	 *  @return void 
	 */
	void Combine(double* u, double* v) {
		//>default: add all child node's input
		*u += *v;
	}

	/** @brief This function evolves the input data type stored in v based on combined value stored in u. It needs to be specialized by the user. EvoID: type of evolve, T: input data type, R: combined type
	 *  @param u pointer to the data structure storing the combined value
	 *  @param v pointer to the data structure storing the input value
	 *  @return void 
	 */
	void Evolve(double* u, double* v) {
		//>default: set the new input to be the old combine
		v = u;
	}


	/** @brief This function adapts user defined combine into tree combine. ComID: type of combine, T: input data type, R: combined type
	 *  @param u pointer to the data structure storing the combined value
	 *  @param v pointer to the data structure storing the input value
	 *  @return void 
	 */
	void TreeCombine(NodePtr u, NodePtr v){
		Combine(u->pCom, v->pVar);
		u->dGen--; //< decrease the gen counter and the own counter
		v->dOwn--; //< a tree node with dOwn = 0 is considered to be removed from the residual tree
	}

	/** @brief This function collapses the basetree from node u up. T: input data type, R: combined type
	 *  @param u node considered
	 *  @return void
	 */
	void Collapse(NodePtr u){
		if(u->dGen != 0) return;
		auto it = u;
		while(it->parent != nullptr && it->parent->dGen <= 1){
			*(it->parent->pCom) = *(it->pCom); //< store the combined value into its parent
			Remove(it);
			it = it->parent;
		}
	}


	/*------------------------------------------BaseTre----------------------------------*/
	/** @brief This class stores a base type tree. Different types of trees can be derived from this.
	 *  @param T input data type
	 *  @param R combined data type
	 */
	class BaseTree {
		public: 
			NodePtr Root; //< pointer to the root
			NodePtr getCommonAncestor(NodePtr u, NodePtr v); //< get common ancestors of nodes u and v
			virtual void Insert(const double& data_in); //< insert node with input value stored in data_in
			virtual void Remove(NodePtr u); //< remove node u from the residual graph
			void Collapse(NodePtr u); //< collapse node u upward the tree
			NodeList TreeNodeList;  //< stores the entire list of the tree nodes. in case of array implementation, this should be a vector instead of list

			BaseTree(std::ifstream); //< constructs a base tree from an input filestream 
			virtual ~BaseTree(){ }; //< destructor. needs overriding
			void tree_compute(bool, std::string, std::string, std::string); //< calls generate or combine templates. specialized tree_compute can be implemented for different types of trees by overriding
	};

	/** @brief This function do one pass of tree computation involving three steps: generate, combine and evolve. T: input data type, R: combined type
	 *  @param strict if the computation has dependency on the newly combined values
	 *  @param ComID type of combine
	 *  @param EvoID type of evolve
	 *  @param GenID type of generate: default to null
	 *  @return void
	 */
	void BaseTree::tree_compute(){
		//> note: needs to figure out the direction of tree compute based on generate
		if(GenID == "null"){
			if(false) {
				//std::string dummy_str = "some node has node.genset -> nullptr"
				//throw UnknownGenerateSetException;
			}
		}else{
			for(auto it = TreeNodeList.begin(); it != TreeNodeList.end(); it++){
				(*it)->pGen->clear(); //< clear the generate and owner sets for all nodes before inserting
				(*it)->pOwn->clear();
			}
			for(auto it = TreeNodeList.begin(); it != TreeNodeList.end(); it++)
				(*it)->getGenSet(Generate<GenID, T, R>(*it)); //have all gensets of nodes figured out
			if(strict) TreeNodeList.sort(); //< sort needs to be overloaded: this sorting needs to reflect how the tree nodes are removed from the residual tree
			for(auto it = TreeNodeList.begin(); it != TreeNodeList.end(); it++){
				for(auto itt = (*it)->pGen.begin(); itt != (*it)->pGen.end(); itt++){
					TreeCombine<ComID, T, R>(*it, *itt); //< call tree combine with dependency
					//< note: necessary markings need to be done during combine
					if(!(*it->dGen)){ //< all generates are considered for it
						if(strict) Evolve<EvoID, T, R>(*it); //< update pVar with the combined value for positive dependency case
						Collapse(*it); //< collapse from *it till a node with dGen > 1
					}
				}
			}
			if(!strict){
				for(auto it = TreeNodeList.begin(); it != TreeNodeList.end(); it++)
					Evolve<EvoID, T, R>(*it); //< update pVar with the combined value all at once for reversed or non dependency cases
			}
		}
	}

	/** @brief This function finds the common ancestor of nodes u and v. T: input data type, R: combined type
	 *  @param u pointer to the first node
	 *  @param v pointer to the second node
	 *  @return NodePtr
	 */
	NodePtr BaseTree::getCommonAncestor(NodePtr u, NodePtr v){
		//...
	}

	/** @brief This function creates and inserts a tree node storing input data value of data_in. T: input data type, R: combined type
	 *  @param data_in pointer to the first node
	 *  @return void
	 */
	void BaseTree::Insert(double* data_in){
		//...
	}	



	/*------------------------------------------OctTree----------------------------------*/

	enum NetworkConstructState
	{
		NAS_LOADING, // loading sequences
		NAS_LOAD_COMPLETE, // loading is complete
		NAS_GEN_ADJ, // generating the sequence data
		NAS_ADJ_COMPLETE, // adjacency generation is complete
		NAS_ERODE, // erode the branch ends one sequence at a time
		NAS_ERODE_WAITING,
		NAS_ERODE_COMPLETE,
		NAS_TRIM, // trimming the data
		NAS_REMOVE_MARKED, // remove marked sequences
		NAS_COVERAGE, // remove low-coverage contigs
		NAS_COVERAGE_COMPLETE,
		NAS_DISCOVER_BUBBLES, // discover read errors/SNPs
		NAS_POPBUBBLE, // remove read errors/SNPs
		NAS_MARK_AMBIGUOUS, // mark ambiguous branches
		NAS_SPLIT_AMBIGUOUS, // split ambiguous branches
		NAS_CLEAR_FLAGS, // clear the flags
		NAS_ASSEMBLE, // assembling the data
		NAS_ASSEMBLE_COMPLETE, // assembling is complete
		NAS_WAITING, // non-control process is waiting
		NAS_DONE // finished, clean up and exit
	};

	class OctTree : public BaseTree{
		//implement specialized methods for OctTree
		public:
			OctTree()
				:m_state(NCT_WAITING);
			void add();
			void remove();
			
			// Receive and dispatch packets;
			size_t pumpNetwork();
			size_t pumpFlushReduce();
			
			

		private:
			void parseControlMessage(int);

			MessageBuffer m_comm;
			//state of construct 
			NetworkConstructState m_state;
			//used by control process to determine state
			unsigned m_numReachedCheckPoint;
	};	

	/*  Receive and dispatch packets
	 *  @return the number of packets received
	 */
	size_t OctTree::pumpNetwork(){
		for(size_t count = 0;  ;count++){
			int senderID;
			APMessage msg = m_comm.checkMessage(senderID);
			switch(msg)
			{
				case APM_CONTROL:
					parseControlMessage(senderID);
					return ++count;
				case APM_BUFFED:
					{
						MessagePtrVector msgs;
						// < receive buffed message, need to do pointer calculation
						// < which is challenging
						m_comm.receiveBufferedMessage(msgs);
						for(auto iter = msg.begin(); 
								iter != megs.end();iter++;){
							//handle each message based on its type
							(*iter)->handle(senderID, *this);
							//delete the message
							delete(*iter);
							*iter = 0;
						}
						break;
					}
				case APM_NONE:
					return count;
			}
		}
	}

	/*  Receive and dispatch packets
	 *  @return the number of packets received
	 */
	void OctTree::parseControlMessage(int source){
		ControlMessage controlMsg = m_comm.receiveControlMessage();
		switch(controlMsg.msgType)
		{
			case APC_SET_STATE:
				SetState(NetworkAssemblyState(controlMsg.argument));
				break;
			case APC_CHECKPOINT:
				m_numReachedCheckpoint++;
				m_checkpointSum += controlMsg.argument;
				break;
			case APC_WAIT:
				SetState(NAS_WAITING);
				m_comm.barrier();
				break;
			case APC_BARRIER:
				assert(m_state == NAS_WAITING);
				m_comm.barrier();
				break;
		}
	}
}
#endif
