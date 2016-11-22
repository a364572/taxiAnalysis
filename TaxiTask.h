#ifndef __TAXITASK_H_
#define __TAXITASK_H_

#include<map>
#include<set>
#include<vector>
#include<string>
#include "Point.h"
#include "Station.h"
#include<pthread.h>
#define MIN_TRIP_TIME 10
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
    virtual ~TaxiTask();

    Point point;
    std::string card;
    int zoneIndex;

    static std::map<std::string, Station> *stationMap;
    static std::map<std::string, std::map<int, std::set<std::string>>> *zoneMap;
    static pthread_mutex_t mutex;

};

class MapTask : public AbstractTask
{
public:
    MapTask(std::string);
    virtual void run();

    //key: 起点经纬度_终点经纬度 value:<key: 时间段 value: vector<int> 时间> 
    static std::map<std::string, std::map<int, std::vector<int>>> *zoneMap;
    static pthread_mutex_t mutex;

    static std::string get_round(std::string);
    std::string record;
};
#endif
