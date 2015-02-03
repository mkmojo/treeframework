#ifndef LOCALOCTTREEARRAY_H
#define LOCALOCTTREEARRAY_H 1

#include<vector>
#include "Point.h"

//Data part for LocalOctTree
class LocalOctTreeArray: public BaseTree
{
    public:
        void add(const Point& seq);
        //Not a localTree nothing to do here
        size_t  pumpNetwork(){return 0;}
    private:
        vector<Point> m_data;
};
#endif
