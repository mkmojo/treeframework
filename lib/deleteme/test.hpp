#include<memory>
#include<vector>
#include<iostream>
namespace treedef {
    template < typename U, typename V> class TreeNode; 
    template < typename U, typename V> using NodePtr = std::shared_ptr<TreeNode<U, V> >;
    template < typename U, typename V> using NodeList = std::vector<std::shared_ptr<TreeNode<U,V> > >;

    template < typename U, typename V> class TreeNode {
        int dGen;
        int dOwn;

        public:
        TreeNode();
        ~TreeNode();
        void getGenSet(NodeList<U, V>*);
    };
    template <typename U, typename V>
        TreeNode<U, V>::TreeNode():dGen(0), dOwn(0) {
            std::cout<< " TreeNode Constructor " << std::endl;
            std::cout<< " dGen is " << dGen <<std::endl;
            std::cout<< " dOwn is " << dOwn <<std::endl;
        }
    template <typename U, typename V>
        TreeNode<U, V>::~TreeNode() {
            std::cout << " destructor " << std::endl;
        }
    template <typename U, typename V>
        void TreeNode<U, V>::getGenSet(NodeList<U,V>* in_list) {
            std::cout << "getGenSet" << std::endl;
        }


    template < typename U, typename V> class BaseClass {
        public:
            virtual void tree_compute(bool)=0;
            BaseClass(){};
            ~BaseClass(){};
    };

    template <typename U, typename V> class OctTree: public BaseClass<U, V> {
        public:
            void printInternal();
            void tree_compute(bool);
            OctTree(){};
            ~OctTree(){};
    };

    template <typename U, typename V> void OctTree<U,V>::tree_compute(bool yo){
        std::cout << " Oct tree_compute bool: " << yo << std::endl;
    }

    template <typename U, typename V> void OctTree<U,V>::printInternal(){
        std::cout << " iterate over internals of the struct "  << std::endl;
    }


}
