#ifndef __TAXITASK_H_
#define __TAXITASK_H_

#include<map>
#include<set>
#include<string>
#include "Point.h"
#include "Station.h"
#include<pthread.h>

class AbstractTask
{
public:
    virtual void run() = 0;
    virtual ~AbstractTask() {}
};

class TaxiTask : public AbstractTask
{
public:
    TaxiTask(Point, std::string, int);
    virtual void run();

    Point point;
    std::string card;
    int zoneIndex;

    static std::map<std::string, Station> *stationMap;
    static std::map<std::string, std::map<int, std::set<std::string>>> *zoneMap;
    static pthread_mutex_t mutex;
};

#endif
