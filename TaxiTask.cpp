#include "TaxiTask.h"
#include "common_function.h"
#include <iostream>
#include <sstream>
#include <limits>
#include <string.h>
using namespace std;

pthread_mutex_t TaxiTask::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MapTask::mutex = PTHREAD_MUTEX_INITIALIZER;
map<string, Station>* TaxiTask::stationMap = nullptr;
map<string, map<int, set<string>>>* TaxiTask::zoneMap = nullptr;
map<string, map<int, vector<int>>>* MapTask::zoneMap = nullptr;
set<std::string>* MapTask::filterSet = nullptr;

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

    string result = to_string(val / 1000) + ".";
    int remain = val % 1000;
    if(remain < 10)
    {
        result += "00" + to_string(remain);
    }
    else if(remain < 100)
    {
        result += "0" + to_string(remain);
    }
    else
    {
        result += to_string(remain);
    }
    return result;
}

class Position
{
public:
    int second;
    Point pos;
    string road;
    Position(string str)
    {
        auto res = split(str, ',');
        second = MapTask::getSecond(res[0]);
        pos = MapTask::getPoint(res[1], res[2]);
        road = res[3];
    }
};
Point MapTask::getPoint(string lat, string lng)
{
    Point point;
    point.latitude = atof(lat.data());
    point.longitude = atof(lng.data());
    return point;
}

int MapTask::getSecond(string val)
{
    struct tm tm;
    strptime(val.c_str(), "%H:%M:%S", &tm); 
    return tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec;
}
/* 统计每个地图方块之间的距离
 * 为了减少误差 对于一个没有搭载乘客的记录 只统计小于5公里的OD
 * 对于搭载乘客的记录 可以全部统计
 * 统计的OD要去除起始或终点在高速路或架桥上的记录
 * */
void MapTask::run()
{
    vector<string> res = split(record, ' ');
    //跳过过短或是没有搭载乘客的记录
    if(res.size() <= 3)
    {
        return;
    }

    bool has_passenger = true;
    if(res[1] == "0")
    {
        has_passenger = false;
        return;
    }

    vector<Position> points;
    auto ite = res.begin();
    ite++;
    ite++;
    while(ite != res.end())
    {
        points.push_back(Position(*ite));
        ite++;
    }

    if(points.rbegin()->second - points.begin()->second >= 3 * 3600)
    {
        return;
    }

    for(vector<Position>::size_type i = 0; i < points.size(); i++)
    {
        if(filterSet->find(points[i].road) != filterSet->end())
        {
            continue;
        }

        auto end_i  = i + 1;
        while(end_i < points.size() && points[end_i].pos == points[i].pos)
        {
            end_i++;
        }
        
        //取中间的记录
        Position begin = points[(i + end_i - 1) / 2];
        i = end_i - 1;

        float distance = 0;
        //如果目的地一样 分别统计最小 中间 和最长 三个值
        for(vector<Position>::size_type j = end_i; j < points.size(); j++)
        {
            Position end = points[j];
            int interval = end.second - begin.second;
            distance += end.pos.getDistance(points[j - 1].pos);
            float direct_distance = end.pos.getDistance(begin.pos);
            float speed = distance / interval;

            //没有乘客 距离不能过长
            //if(!has_passenger && distance > MAX_OD_DISTANCE)
            //{
            //    break;
            //}

            //OD时间 距离 速度都不能过小
            if(interval < MIN_OD_TIME || speed < MIN_OD_SPEED ||
                    distance < MIN_OD_DISTANCE || 
                    direct_distance < MIN_OD_DISTANCE)
            {
                continue;
            }

            //OD起始地和目的地不能是高速路 架桥等
            if(filterSet->find(end.road) != filterSet->end())
            {
                continue;
            }

            //经纬度转换失败
            string lat1 = get_round(to_string(begin.pos.latitude));
            string lng1 = get_round(to_string(begin.pos.longitude));
            string lat2 = get_round(to_string(end.pos.latitude));
            string lng2 = get_round(to_string(end.pos.longitude));
            if(lat1.size() == 0 || lng1.size() == 0 ||
                    lat2.size() == 0 || lng2.size() == 0)
            {
                break;
            }

            //如果目的地在之前的路径中出现过 说明绕路了
            bool cheat = false;
            for(auto k = end_i; k < j; k++)
            {
                if(points[k].pos == end.pos)
                {
                    cheat = true;
                    break;
                }
            }
            if(cheat)
            {
                break;
            }

            string od = lat1 + "," + lng1 + "_" + lat2 + "," + lng2;
            //向后搜寻所有附近的点
            auto end_j = j + 1;
            while(end_j < points.size() && points[end_j].pos == end.pos)
            {
                end_j++;
            }

            int midInterval = 0;
            int maxInterval = 0;
            float midSpeed = speed;
            float maxSpeed = speed;
            interval = interval / 60 + 1;
            midInterval = (points[(end_j + j - 1) / 2].second - begin.second) / 60 + 1;
            maxInterval = (points[end_j - 1].second - begin.second) / 60 + 1;

            midSpeed = (distance + points[(end_j + j - 1) / 2].pos.getDistance(end.pos)) / midInterval;
            maxSpeed = (distance + points[end_j - 1].pos.getDistance(end.pos)) / maxInterval;

            j = end_j - 1;

            int timeindex = (begin.second - 6 * 3600) / ZONE_INTERVAL;

            //cout << begin.pos.latitude << "," << to_string(begin.pos.longitude) << " "
            //    << end.pos.latitude << "," << to_string(end.pos.longitude) << " " << 
//            cout << od << " " << timeindex << " " << distance << " " << interval << " " << speed << endl;


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
            if(midInterval != interval && midSpeed > MIN_OD_SPEED)
            {
                zoneMap->at(od)[timeindex].push_back(midInterval);
            }
            if(maxInterval != midInterval && maxSpeed > MIN_OD_SPEED)
            {
                zoneMap->at(od)[timeindex].push_back(maxInterval);
            }
            pthread_mutex_unlock(&mutex);
        }
    }
};
