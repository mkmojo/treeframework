#include "Algorithm.h"
#include <algorithm>
#include <climits>
#include <iostream>
#include <vector>
#include "treedef.h"
#include "Log.h"

namespace FMMAlgorithms
{
    void loadNodes(LocalOctTree* localOctTree, string inFile)
    {
        Timer timer("LoadSequences " + inFile);
        logger(0) << "Reading `" << inFile << "'...\n";
       // Reader reader(inFile.c_str(), readerMode);
       // for(InRecord rec; reader >> red; ) {
       //    localOctTree->add(TreeNode(rec));
       // }
    }
}
