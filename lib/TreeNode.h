#ifndef TREENODE_H
#define TREENODE_H 1

#include "Common/Options.h"
#include <vector>
using namespace std;

class Point
{
    public:
        double x;
        double y;
        double z;
        unsigned int x_key;
        unsigned int y_key;
        unsigned int z_key;
        double mass;
        unsigned getNodeID() const;
};

unsigned Point::getNodeID() const
{
    // number of bits required for each x, y and z = height of the tree = levels_ - 1
    // therefore, height of more than 21 is not supported
    unsigned int height = level - 1;

    uint64_t key = 1;
    uint64_t x_64 = (uint64_t) x_key << (64 - height);
    uint64_t y_64 = (uint64_t) y_key << (64 - height);
    uint64_t z_64 = (uint64_t) z_key << (64 - height);
    uint64_t mask = 1;
    mask = mask << 63; // leftmost bit is 1, rest 0

    for(unsigned int i = 0; i < height; ++i) {
        key = key << 1;
        if(x_64 & mask) key = key | 1;
        x_64 = x_64 << 1;

        key = key << 1;
        if(y_64 & mask) key = key | 1;
        y_64 = y_64 << 1;

        key = key << 1;
        if(z_64 & mask) key = key | 1;
        z_64 = z_64 << 1;
    } // for

    return key; //this is the key for cell it belongs to
}

class TreeNode
{
    public:
        TreeNode(){}
        explicit TreeNode(const Point& seq);
        unsigned NodeID = 0;
        void addPoint(Point& p);

    private:
        vector<Point> points;
};

#endif
