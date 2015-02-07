#ifndef BASETREE_H
#define BASETREE_H 1
#include "Common/Log.h"
#include "MessageBuffer.h"
#include "Point.h"

/* -------------------------- BaseTree --------------------------- */
class BaseTree
{
    public:
        virtual ~BaseTree() {}
        virtual void add(const Point& p) = 0;
        virtual size_t pumpNetwork() = 0;
};
#endif
