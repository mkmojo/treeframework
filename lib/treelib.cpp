#include "treelib.hpp"

//Define the destructor
template< typename T, typename R>
OctTree<T, R>::~OctTree() {
    //Deallocate the memory used 
    delete root;
}
