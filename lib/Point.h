#ifndef POINT_H
#define POINT_H 1

#include "Common/Options.h"
#include <vector>
using namespace std;

class Point
{
    public:
        double x;
        double y;
        double z;
        double mass;
        explicit Point(double x, double y, double z):x(x), y(y), z(z){}


        unsigned x_key;
        unsigned y_key;
        unsigned z_key;
        unsigned cell_key;

        unsigned getCellID() const;
};

#endif
