#include "Algorithm.h"
#include <algorithm>
#include <climits>
#include <iostream>
#include <sstream>
#include <vector>
#include "treedef.h"
#include "Log.h"
using namespace std;
namespace FMMAlgorithms
{
    void loadNodes(LocalOctTree* localOctTree, string inFile)
    {
        Timer timer("LoadSequences " + inFile);
        logger(0) << "Reading `" << inFile << "'...\n";
        ifstream particleFile(inFile);

        if(particleFile.is_open()) {
            string line;
            double x, y, z;
            while(getline (particleFile, line)){
                stringstream sstr(line);
                cout << line << endl;
                sstr >> x >> y >> z >> mass;
                LocalOctTree->add(Point(x, y, z, mass));
            }
        }else{
            cout<< "Unable to open file" << endl;
        }
    }
};
