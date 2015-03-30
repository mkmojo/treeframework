#include "Cell.h"

bool Cell::operator==(const Cell &c1, const Cell &c2)
{
    return c1.cell_id == c2.cell_id;
}

std::ostream& Cell::operator<<(std::ostream &os, const Cell &obj)
{
    os<<obj.cell_id;
    return os;
}
