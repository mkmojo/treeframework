#include "LocalOctTreeArray.h"

/** add a specific point to this collection **/
LocalOctTreeArray::add(const Point& seq)
{
    m_data.push_back(seq);
}
