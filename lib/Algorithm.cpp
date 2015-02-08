#include "Algorithm.h"
#include <algorithm>
#include <climits>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include "LocalOctTree.h"
#include "Common/Log.h"
using namespace std;
namespace FMMAlgorithms
{
    void loadPoints(LocalOctTree* localOctTree, string inFile)
    {
        //Timer timer("LoadSequences " + inFile);
        //logger(0) << "Reading `" << inFile << "'...\n";
        ifstream particleFile("inFile.txt"); // This need to be modifed to take in any file name

        if(particleFile.is_open()) {
            string line;
            double x, y, z, mass;
            while(getline (particleFile, line)){
                stringstream sstr(line);
                //cout << line << endl;
                sstr >> x >> y >> z >> mass;
                localOctTree->add(Point(x, y, z, mass));
            }
        }else{
            cout<< "Unable to open file" << endl;
        }
    }
};
