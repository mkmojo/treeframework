#include "test.hpp"
#include <vector> 
#include <iostream>
#include <memory>
int main()
{   
    using namespace treedef;
    NodePtr<double, double> pMynode;
    NodeList<double, double> lst = {pMynode};
    std::cout << pMynode <<std::endl;
    std::cout << lst.size() <<std::endl;
    TreeNode<double, double> sampleNode;
    sampleNode.getGenSet(&lst);
    OctTree<double, double> hey_a_tree;
    return 0;
}
