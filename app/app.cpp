#include "../lib/treedef.hpp" //< should not be here for a runtime system
#include "../lib/treelib.hpp" //< should not be here for a runtime system
#include "userlib.hpp"
#include <fstream>

using namespace treedef; //< should not be here for a runtime system

//Application Code
int main(){

    //read in initial data. can be modularized
    std::ifstream initial_data_cube("data_file.dat");

    //construct the tree
    OctTree<Input_Data_Type, Combined_Data_Type> data_tree(initial_data_cube);
    
    while(loop_condition()){
        data_tree.tree_compute(true, "UserCombine1", "UserEvolve1", "UserGenerate");
        data_tree.tree_compute(true, "UserCombine2", "UserEvolve1");
    }
}
