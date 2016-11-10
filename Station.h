#ifndef __STATION_H_
#define __STATION_H_
#include<string>
#include "Point.h"
class Station {
public:
	std::string name;
	Point position;
    int traffic;

    Station() : traffic(0)
    {
    }

};
#endif
