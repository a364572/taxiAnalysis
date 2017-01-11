#ifndef __TAXITASK_H_
#define __TAXITASK_H_

#include<map>
#include<set>
#include<vector>
#include<string>
#include "Point.h"
#include "Station.h"
#include "common_function.h"
#include<pthread.h>
class AbstractTask
{
public:
    virtual void run() = 0;
    virtual ~AbstractTask() {}
    static std::string get_round(std::string);
    static Point getPoint(std::string, std::string);
    static int getSecond(std::string);
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
    static std::set<std::string> *filterSet;
    static pthread_mutex_t mutex;

    std::string record;
};

//统计每个区域每个小时的出租车数
class TimeTask : public AbstractTask
{
public:
    TimeTask(std::string line) : record(line) {}
    virtual void run();

    //key: 起点经纬度_终点经纬度 value:<key: 时间段 value: 计数 时间> 
    static std::map<std::string, std::map<int, std::set<std::string>>> *zoneMap;
    static std::set<std::string> *filterSet;
    static pthread_mutex_t mutex;

    std::string record;
};

//实验
class ResultTask : public AbstractTask
{
public:
    ResultTask(std::string line) : record(line) {}
    virtual void run();

    static std::map<std::string, Station>* stationMap;
    static std::map<std::string, std::map<int, Info>>* subwayMap;
    static std::map<std::string, std::map<int, Info>>* waitTaxiMap;
    static std::map<std::string, std::map<int, TripTime>>* taxiODMap;
    static std::map<std::string, int>* subwayPriceMap;
    static pthread_mutex_t mutex;
    static int fd;

    static std::string getNeareastStation(std::string, float&);
    static Info getInfoOfTaxi(std::string, std::string, int); 
    static Info getInfoOfSubway(std::string, std::string, int); 
    static void calculateMinTime(std::string, std::string);
    static bool compareInfo(Info&, Info&, int);

    std::string record;
};

//最短路径
class ShortestPathTask : public AbstractTask
{
public:
    int hour;
    std::string point;
    ShortestPathTask(int h, std::string param) : hour(h), point(param) {}
    virtual void run();

    static std::set<std::string>* points;
    static std::map<int, std::map<std::string, float>>* directMap;
    static std::map<int, std::map<std::string, float>>* totalRouteMap;
    static pthread_mutex_t mutex;
};
#endif
