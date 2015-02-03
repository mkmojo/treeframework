#ifndef TREENODE_H
#define TREENODE_H 1

#include "Common/Options.h"
#include <vector>
using namespace std;

class Point
{
    double x;
    double y;
    double z;
    double mass;
};

class TreeNode
{
    public:
        TreeNode(){}
        explicit TreeNode(const Point& seq);
        unsigned getNodeID(int ) const;

    private:
        vector<Point> points;
};

#endif
