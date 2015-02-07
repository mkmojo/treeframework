#ifndef POINT_H
#define POINT_H 1

#include "Common/Options.h"
#include <vector>
#include <cstring> // for memcpy
#define NUM_SLOT 4 // Need to be changed to adjust automatically

using namespace std;

class Point
{
    public:
        double x, y, z;
        double mass;
        explicit Point(double x, 
                double y, double z, double mass)
            :x(x), y(y), z(z), mass(mass){
                memcpy(m_point, &x, sizeof x);
                memcpy(m_point + sizeof(double), &y, sizeof y);
                memcpy(m_point + sizeof(double)*2, &z, sizeof z);
                memcpy(m_point + sizeof(double)*3, &mass, sizeof mass);
            }
        static unsigned serialSize() {return NUM_BYTES;}
        //copy point data to memory location starting from dest;
        size_t serialize(void* dest) const
        {
            memcpy(dest, m_point, sizeof m_point);
            return sizeof m_point;
        }

        //copy bytes from src buffer region to point m_point char arrary
        size_t unserialize(const void* src)
        {
            //This is now ugly, should be done through a function
            memcpy(&x, m_point, sizeof(double));
            memcpy(&y, m_point + sizeof(double), sizeof y);
            memcpy(&z, m_point + sizeof(double) * 2, sizeof z);
            memcpy(&mass, m_point + sizeof(double) * 3, sizeof mass);

            memcpy(m_point, src, sizeof m_point);
            return sizeof m_point;
        }
        static const unsigned NUM_BYTES =  sizeof(double) * NUM_SLOT;
        void print_m_point()
        {
            double x;
            for( int i = 0; i < NUM_SLOT ;i++){
                memcpy(&x, &m_point[i*sizeof(double)], sizeof x);
                printf("%lf\n",x);
            }
        };
    protected:
        char m_point[NUM_BYTES];
};
#endif

