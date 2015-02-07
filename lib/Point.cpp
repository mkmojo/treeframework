#include "Point.h"

//unsigned Point::getCellID() const
//{
//    return 1;
//}

unsigned Point::getCode() const
{
    // use a has function to decide where this should go
    // DEBUG: this need to change with different input structure
    const unsigned prime = 101;
    unsigned sum = 0;
    for(unsigned i = 0 ;i < NUM_BYTES; i++)
        sum = prime * sum + m_point[i];
    return sum;
}
