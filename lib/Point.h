#ifndef POINT_H
#define POINT_H 1

#include "Common/Options.h"
#include <vector>
#include <cstring> //for memcpy

using namespace std;

class Point
{
    public:
        double x, y, z;
        double mass;
        explicit Point(double x, 
                double y, double z, double mass)
            :x(x), y(y), z(z), mass(mass){}

        unsigned x_key;
        unsigned y_key;
        unsigned z_key;
        unsigned cell_key;

        unsigned getCellID() const;
        int getCode();
        size_t serialize(void* dest) const
        {
            memcpy(dest, m_point, sizeof m_point);
            return sizeof m_point;
        }

        size_t unserialize(const void* src)
        {
            memcpy(m_point, src, sizeof m_point);
            return sizeof m_point;
        }
        static const unsigned NUM_BYTES =  sizeof(double) * 4;
    protected:
        char m_point[NUM_BYTES];
}; 
#endif
    
