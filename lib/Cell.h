#ifndef CELL_H
#define CELL_H 1
#include <vector>
#include <iostream>
#include "Point.h"

class Cell{
    public:
        long cell_id;
        std::vector<Point> points;

        friend bool operator==(const Cell &c1, const Cell &c2) 
        {
            return c1.cell_id == c2.cell_id;
        }
        friend std::ostream& operator<<(std::ostream &os, const Cell &obj)
        {
            os << obj.cell_id;
            return os;
        }

        Cell(long cell_id, Point &p)
            :cell_id(cell_id)
        {
            points.push_back(p);
        }
        Cell(long cell_id):cell_id(cell_id) {}
};
#endif
