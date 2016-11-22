#include "TaxiTask.h"
#include "common_function.h"
#include <iostream>
#include <sstream>
#include <string.h>
using namespace std;

pthread_mutex_t TaxiTask::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MapTask::mutex = PTHREAD_MUTEX_INITIALIZER;
map<string, Station>* TaxiTask::stationMap = nullptr;
map<string, map<int, set<string>>>* TaxiTask::zoneMap = nullptr;
map<string, map<int, vector<int>>>* MapTask::zoneMap = nullptr;

TaxiTask::TaxiTask(Point p, string c, int i) :
   point(p), card(c), zoneIndex(i)
{
}
void TaxiTask::run()
{
    auto ite = stationMap->begin();
    while(ite != stationMap->end())
    {
        auto &set = zoneMap->at(ite->first).at(zoneIndex);
        if(set.find(card) == set.end())
        {
            auto distance = point.getDistance(ite->second.position);
            if (distance <= MAX_DISTANCE)
            {
                pthread_mutex_lock(&mutex);
                set.insert(card);
                pthread_mutex_unlock(&mutex);
            }
        }
        ite++;
    }
}

TaxiTask::~TaxiTask()
{
}

MapTask::MapTask(string str) : record(str)
{
}
string MapTask::get_round(string str)
{
    stringstream ss;
    float value;
    ss << str;
    ss >> value;

    int val = value * 1000;
    val = val - val % GRANULARITY;

    if(val < 100000)
    {
        if(val < MIN_LATITUDE || val > MAX_LATITUDE)
        {
            return "";
        }
    }
    else
    {
        if(val < MIN_LONGITUDE || val > MAX_LONGITUDE)
        {
            return "";
        }
    }
    return to_string(val / 1000) + "." + to_string(val % 1000);
}
//统计每个地图方块之间的距离
void MapTask::run()
{
    vector<string> res = split(record, ' ');
    //跳过过短或是没有搭载乘客的记录
    if(res.size() <= 3 || res[1] == "0")
    {
        return;
    }

    string item_start = res[2];
    string item_end = *(res.end() - 1);
    vector<string> res_start = split(item_start, ',');
    vector<string> res_end = split(item_end, ',');

    struct tm tm_start;
    struct tm tm_end;
    strptime(res_start[0].c_str(), "%H:%M:%S", &tm_start);
    strptime(res_end[0].c_str(), "%H:%M:%S", &tm_end);

    int interval = (tm_end.tm_hour - tm_start.tm_hour) * 60 +
        tm_end.tm_min - tm_start.tm_min;
    if(interval < MIN_TRIP_TIME) 
    {
        return;
    } 

    string lat1 = get_round(res_start[1]);
    string lng1 = get_round(res_start[2]);
    string lat2 = get_round(res_end[1]);
    string lng2 = get_round(res_end[2]);
    if(lat1.size() == 0 || lng1.size() == 0 ||
           lat2.size() == 0 || lng2.size() == 0)
    {
        return;
    }
    string od = lat1 + "," + lng1 + "_" + lat2 + "," + lng2;

    int timeindex = ((tm_start.tm_hour - 6) * 3600 + tm_start.tm_min * 60 +
        tm_start.tm_sec) / ZONE_INTERVAL;

    pthread_mutex_lock(&mutex);
    if (zoneMap->find(od) == zoneMap->end())
    {
        (*zoneMap)[od] = map<int, vector<int>>();
    }
    if (zoneMap->at(od).find(timeindex) == zoneMap->at(od).end())
    {
        zoneMap->at(od)[timeindex] = vector<int>();
    }
    zoneMap->at(od)[timeindex].push_back(interval);
    pthread_mutex_unlock(&mutex);
};
