#include "test.hpp"
#include <vector> 
#include <iostream>
#include <memory>
using namespace std;
int main()
{   
    using namespace treedef;
    NodePtr<double, double> pMynode;
    NodeList<double, double> lst = {pMynode};
    cout << pMynode <<endl;
    cout << lst.size() <<endl;
    return 0;
}
