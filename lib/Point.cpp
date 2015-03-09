#include "Point.h"
#include <string>
using namespace std;

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

void Point::print_m_point()
{
    double x;
    for( int i = 0; i < NUM_SLOT ;i++){
        memcpy(&x, &m_point[i*sizeof(double)], sizeof x);
        printf("%.2lf ",x);
    }
    printf("\n");
}

void Point::print_cord(){
    printf("%.2lf ",x);
    printf("%.2lf ",y);
    printf("%.2lf ",z);
    printf("\n");
}

