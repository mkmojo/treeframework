#include "LocalOctTreeArray.h"

/** add a specific point to this collection **/
LocalOctTreeArray::add(const Point& p)
{
    m_data.push_back(p);
}
