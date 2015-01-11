#include "../treedef.hpp"
int main()
{
    treedef::OctTree<double, double> myoct;
    std::cout << myoct.setCounter(10.0) <<std::endl;
    return 0;
}
