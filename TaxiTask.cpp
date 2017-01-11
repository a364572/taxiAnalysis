#include "TaxiTask.h"
#include "common_function.h"
#include <iostream>
#include <sstream>
#include <limits>
#include <string.h>
#include <unistd.h>
using namespace std;

pthread_mutex_t TaxiTask::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MapTask::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t TimeTask::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ResultTask::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ShortestPathTask::mutex = PTHREAD_MUTEX_INITIALIZER;

map<string, Station>* TaxiTask::stationMap = nullptr;
map<string, map<int, set<string>>>* TaxiTask::zoneMap = nullptr;

map<string, map<int, vector<int>>>* MapTask::zoneMap = nullptr;
set<std::string>* MapTask::filterSet = nullptr;

map<string, map<int, set<string>>>* TimeTask::zoneMap = nullptr;
set<std::string>* TimeTask::filterSet = nullptr;

map<string, Station>* ResultTask::stationMap = nullptr;
map<string, map<int, Info>>* ResultTask::subwayMap = nullptr;
map<string, map<int, Info>>* ResultTask::waitTaxiMap = nullptr;
map<string, map<int, TripTime>>* ResultTask::taxiODMap = nullptr;
map<string, int>* ResultTask::subwayPriceMap = nullptr;
int ResultTask::fd = 0;

set<string>* ShortestPathTask::points = nullptr;
map<int, map<string, float>>* ShortestPathTask::directMap = nullptr;
map<int, map<string, float>>* ShortestPathTask::totalRouteMap = nullptr;

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
string AbstractTask::get_round(string str)
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
    return to_string(val);
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
Point AbstractTask::getPoint(string lat, string lng)
{
    Point point;
    point.latitude = atof(lat.data());
    point.longitude = atof(lng.data());
    return point;
}

int AbstractTask::getSecond(string val)
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

//    bool has_passenger = true;
    if(res[1] == "0")
    {
//        has_passenger = false;
//        return;
    }

    if(Position(*(res.rbegin())).second / 3600 < MIN_HOUR)
    {
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

    if((points.rbegin()->second - points.begin()->second) >= 3 * 3600)
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

        //逗留时间过长的记录
        if(points[end_i - 1].second - points[i].second > 600)
        {
            i = end_i - 1;
            continue;
        }
        
        //取中间的记录
        Position begin = points[(i + end_i - 1) / 2];
        i = end_i - 1;

        //只统计16点以后的记录
        if(begin.second / 3600 < MIN_HOUR || begin.second / 3600 >= MAX_HOUR)
        {
            continue;
        }


        float distance = 0;
        //如果目的地一样 分别统计最小 中间 和最长 三个值
        for(vector<Position>::size_type j = end_i; j < points.size(); j++)
        {
            //目的地和起始地一样 绕路了
            Position end = points[j];
            if(end.pos == begin.pos)
            {
                break;
            }
            int interval = end.second - begin.second;
            distance += end.pos.getDistance(points[j - 1].pos);
            //float direct_distance = end.pos.getDistance(begin.pos);
            float speed = distance / interval;

            //没有乘客 距离不能过长
            //if(!has_passenger && distance > MAX_OD_DISTANCE)
            //{
            //    break;
            //}
            

            //OD时间 距离 速度都不能过小
            //if(interval < MIN_OD_TIME || speed < MIN_OD_SPEED ||
            //        distance < MIN_OD_DISTANCE || 
            //        direct_distance < MIN_OD_DISTANCE)
            //{
            //    continue;
            //}


            string lat1 = get_round(to_string(begin.pos.latitude));
            string lng1 = get_round(to_string(begin.pos.longitude));
            string lat2 = get_round(to_string(end.pos.latitude));
            string lng2 = get_round(to_string(end.pos.longitude));

            //经纬度转换失败
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

            string od = lat1 + "," + lng1 + " " + lat2 + "," + lng2;
            //向后搜寻所有附近的点
            auto end_j = j + 1;
            while(end_j < points.size() && points[end_j].pos == end.pos)
            {
                end_j++;
            }

            int midInterval = 0;
            int maxInterval = 0;
            float midSpeed = 0;
            float maxSpeed = 0;
            interval = interval / 60 + 1;
            midInterval = (points[(end_j + j - 1) / 2].second - begin.second) / 60 + 1;
            maxInterval = (points[end_j - 1].second - begin.second) / 60 + 1;

            midSpeed = (distance + points[(end_j + j - 1) / 2].pos.getDistance(end.pos)) / midInterval;
            maxSpeed = (distance + points[end_j - 1].pos.getDistance(end.pos)) / maxInterval;

            if(end_j != j + 1)
            {
                distance += points[end_j - 1].pos.getDistance(end.pos);
            }
            j = end_j - 1;

            //OD起始地和目的地不能是高速路 架桥等
            if(filterSet->find(end.road) != filterSet->end())
            {
                continue;
            }

            int timeindex = (begin.second - 6 * 3600) / ZONE_INTERVAL;

            //cout << begin.pos.latitude << "," << to_string(begin.pos.longitude) << " "
            //    << end.pos.latitude << "," << to_string(end.pos.longitude) << " " << 
//            cout << od << " " << timeindex << " " << distance << " " << interval << " " << speed << endl;


            if(speed < MIN_OD_SPEED || midSpeed < MIN_OD_SPEED || maxSpeed < MIN_OD_SPEED)
            {
                break;
            }
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
void TimeTask::run()
{
    vector<string> res = split(record, ' ');
    //跳过过短或是没有搭载乘客的记录
    if(res.size() <= 3)
    {
        return;
    }

    //不考虑有乘客的出租车
    if(res[1] == "1")
    {
//        return;
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

    for(vector<Position>::size_type i = 0; i < points.size(); i++)
    {

        Position begin = points[i];
        //OD起始地和目的地不能是高速路 架桥等
        if(filterSet->find(begin.road) != filterSet->end())
        {
            continue;
        }

        //经纬度转换失败
        string lat1 = get_round(to_string(begin.pos.latitude));
        string lng1 = get_round(to_string(begin.pos.longitude));
        if(lat1.size() == 0 || lng1.size() == 0 )
        {
            break;
        }

        string od = lat1 + "," + lng1;
        int timeindex = (begin.second) / ZONE_INTERVAL;

        pthread_mutex_lock(&mutex);
        if (zoneMap->find(od) == zoneMap->end())
        {
            (*zoneMap)[od] = map<int, set<string>>();
        }
        if (zoneMap->at(od).find(timeindex) == zoneMap->at(od).end())
        {
            zoneMap->at(od)[timeindex] = set<string>();
        }
        zoneMap->at(od)[timeindex].insert(res[0]);
        pthread_mutex_unlock(&mutex);
    }
}
//得到最近的地铁站
string ResultTask::getNeareastStation(string position, float& min_distance)
{
    auto vec = split(position, ',');
    string lat = vec[0].substr(0, 2) + "." + vec[0].substr(2);
    string lng = vec[1].substr(0, 3) + "." + vec[1].substr(3);
    Point point;
    point.latitude = strtof(lat.data(), NULL);
    point.longitude = strtof(lng.data(), NULL);

    min_distance = 1000000;
    string min_station;
    auto ite = stationMap->begin();
    while(ite != stationMap->end())
    {
        float distance = point.getDistance(ite->second.position);
        if (distance < min_distance)
        {
            min_distance = distance;
            min_station = ite->first;
        }
        ite++;
    }
    return min_station;
}
//得到一个OD通过乘坐出租车所需的时间 包括等车时间和坐车时间
Info ResultTask::getInfoOfTaxi(string src, string dst, int hour)
{
    Info info;
    auto vec1 = split(src, ',');
    auto vec2 = split(dst, ',');
    int latitude1 = atoi(vec1[0].data());
    int longitude1 = atoi(vec1[1].data());
    int latitude2 = atoi(vec2[0].data());
    int longitude2 = atoi(vec2[1].data());
    if(hour >= MAX_HOUR || waitTaxiMap->find(src) == waitTaxiMap->end() || 
            waitTaxiMap->at(src).find(hour) == waitTaxiMap->at(src).end())
    {
        return info;
    }
    float avg = waitTaxiMap->at(src)[hour].avg;
    float dif = waitTaxiMap->at(src)[hour].dif;
    if (avg >= 60)
    {
        hour++;
    }
    vector<string> starts, ends;
    vector<int> diff;
    diff.push_back(0);
    diff.push_back(-GRANULARITY);
    diff.push_back(GRANULARITY);
    for(auto diff1 : diff)
    {
        int lat1 = latitude1 + diff1;
        int lat2 = latitude2 + diff1;
        for(auto diff2 : diff)
        {
            int lng1 = longitude1 + diff2;
            int lng2 = longitude2 + diff2;
            string region1 = (to_string(lat1) + "," + to_string(lng1));
            string region2 = (to_string(lat2) + "," + to_string(lng2));
            starts.push_back(region1);
            ends.push_back(region2);
        }
    }

    for(auto start : starts)
    {
        for(auto end : ends)
        {
            string od = start + " " + end;
            if(taxiODMap->find(od) != taxiODMap->end() &&
                    taxiODMap->at(od).find(hour) != taxiODMap->at(od).end())
            {
                avg += taxiODMap->at(od)[hour].avg;
                dif += taxiODMap->at(od)[hour].dif;
                info.avg = avg;
                info.dif = dif;
                /**************价格****************/
                info.price = taxiODMap->at(od)[hour].avg;
                return info;
            }
        }
    }
    return info;
}
//得到一个OD通过乘坐地铁所需的时间
Info ResultTask::getInfoOfSubway(string src, string dst, int hour)
{
    Info info;
    float srcDistance, dstDistance;
    string srcStation = getNeareastStation(src, srcDistance);
    string dstStation = getNeareastStation(dst, dstDistance);
    if(srcDistance > 2000 || dstDistance > 2000)
    {
        return info;
    }
    if(srcStation.size() > 0 && dstStation.size() > 0)
    {
        string od = srcStation + " " + dstStation;
        if(subwayMap->find(od) != subwayMap->end() &&
                subwayMap->at(od).find(hour) != subwayMap->at(od).end())
        {
            info.avg = (srcDistance + dstDistance) / WALK_SPEED / 60 +
                subwayMap->at(od)[hour].avg;
            info.dif = subwayMap->at(od)[hour].dif;
            /**************价格****************/
            info.price = subwayPriceMap->at(srcStation + " " + dstStation);
            info.price = 0;
        }
    }
    return info;
}
bool ResultTask::compareInfo(Info& info1, Info& info2, int method)
{
    if(info1.avg < 0)
    {
        return false;
    }
    if(info2.avg < 0)
    {
        return true;
    }
    if(method == METHOD_TIME)
    {
        if(info1.avg < info2.avg)
        {
            return true;
        }
    }
    else
    {
        if(info1.price < info2.price)
        {
            return true;
        }
    }
    return false;
}
void ResultTask::calculateMinTime(string src, string dst)
{
    for(int hour = MIN_HOUR; hour < MAX_HOUR; hour++)
    {
        string od = src + " " + dst;
        if(waitTaxiMap->find(src) == waitTaxiMap->end() || waitTaxiMap->at(src)[hour].avg < 0)
        {
            continue;
        }
        //只搭乘地铁
        Info info1 = getInfoOfSubway(src, dst, hour);
        //只搭乘出租车
        Info info2 = getInfoOfTaxi(src, dst, hour);
        //先坐地铁再坐出租车
        Info info3_time, info3_price;
        for(auto ite : *stationMap)
        {
            string mid = ite.second.positionString;
            Info tmp1 = getInfoOfSubway(src, mid, hour);
            if(tmp1.avg < 0)
            {
                continue;
            }
            int midHour = hour + (tmp1.avg + WALK_TIME) / 60;
            Info tmp2 = getInfoOfTaxi(mid, dst, midHour); 
            if(tmp2.avg < 0)
            {
                continue;
            }
            tmp1.avg += tmp2.avg + WALK_TIME;
            tmp1.dif += tmp2.dif;
            tmp1.price += tmp2.price;
            if(compareInfo(tmp1, info3_time, METHOD_TIME)) 
            {
                info3_time = tmp1;
            }
            if(compareInfo(tmp1, info3_price, METHOD_PRICE))
            {
                info3_price = tmp1;
            }
        }
        //先坐出租车再坐地铁
        Info info4_time, info4_price;
        for(auto ite : *stationMap)
        {
            string mid = ite.second.positionString;
            Info tmp1 = getInfoOfTaxi(src, mid, hour); 
            if(tmp1.avg < 0)
            {
                continue;
            }
            int midHour = hour + (tmp1.avg + WALK_TIME) / 60;
            Info tmp2 = getInfoOfSubway(mid, dst, midHour);
            if(tmp2.avg < 0)
            {
                continue;
            }
            tmp1.avg += tmp2.avg + WALK_TIME;
            tmp1.dif += tmp2.dif;
            tmp1.price += tmp2.price;
            if(compareInfo(tmp1, info4_time, METHOD_TIME)) 
            {
                info4_time = tmp1;
            }
            if(compareInfo(tmp1, info4_price, METHOD_PRICE)) 
            {
                info4_price = tmp1;
            }
        }
        //先坐出租车 再坐地铁 最后再坐出租车
        Info info5_time, info5_price;
        for(auto ite1 : *stationMap)
        {
            string midSrc = ite1.second.positionString;
            Info tmp1 = getInfoOfTaxi(src, midSrc, hour); 
            if(tmp1.avg < 0)
            {
                continue;
            }
            for(auto ite2 : *stationMap)
            {
                string midDst = ite2.second.positionString;
                if(midSrc == midDst)
                {
                    continue;
                }
                int midSrcHour = hour + (tmp1.avg + WALK_TIME) / 60;
                Info tmp2 = getInfoOfSubway(midSrc, midDst, midSrcHour);
                if(tmp2.avg < 0)
                {
                    continue;
                }
                int midDstHour = hour + (tmp1.avg + tmp2.avg + 2 * WALK_TIME) / 60;
                Info tmp3 = getInfoOfTaxi(midDst, dst, midDstHour);
                if(tmp3.avg < 0)
                {
                    continue;
                }

                tmp1.avg += tmp2.avg + tmp3.avg + 2 * WALK_TIME;
                tmp1.dif += tmp2.dif + tmp3.dif;
                tmp1.price += tmp2.price + tmp3.price;
                if(compareInfo(tmp1, info5_time, METHOD_TIME)) 
                {
                    info5_time = tmp1;
                }
                if(compareInfo(tmp1, info5_price, METHOD_PRICE)) 
                {
                    info5_price = tmp1;
                }
            }
        }
        Info info_time, info_price; 
        int index_time = 0, index_price = 0;
        if(compareInfo(info1, info_time, METHOD_TIME))
        {
            info_time = info1;
            index_time = 1;
        }
        if(compareInfo(info3_time, info_time, METHOD_TIME))
        {
            info_time = info3_time;
            index_time = 3;
        }
        if(compareInfo(info4_time, info_time, METHOD_TIME))
        {
            info_time = info4_time;
            index_time = 4;
        }
        if(compareInfo(info5_time, info_time, METHOD_TIME))
        {
            info_time = info5_time;
            index_time = 5;
        }
        if(compareInfo(info1, info_price, METHOD_PRICE))
        {
            info_price = info1;
            index_price = 1;
        }
        if(compareInfo(info3_price, info_price, METHOD_PRICE))
        {
            info_price = info3_price;
            index_price = 3;
        }
        if(compareInfo(info4_price, info_price, METHOD_PRICE))
        {
            info_price = info4_price;
            index_price = 4;
        }
        if(compareInfo(info5_price, info_price, METHOD_PRICE))
        {
            info_price = info5_price;
            index_price = 5;
        }
        if(fd > 0)
        {
            string out = src + " " + dst + " " + to_string(hour) + " " 
                + to_string(index_time) + " " + to_string(index_price) + " " 
                + to_string(info_time.avg) + " " + to_string(info_price.avg) + " "
                + to_string(info_time.price) + " " + to_string(info_price.price) + " "
                + to_string(info2.avg) + " " + to_string(info2.price) + "\n";
            pthread_mutex_lock(&mutex);
            write(fd, out.data(), out.size());
            pthread_mutex_unlock(&mutex);
        }
    }
}

void ResultTask::run()
{
    auto vec = split(record, ' ');
    if(checkPosition(vec[0]) || checkPosition(vec[1]))
    {
        return;
    }
    calculateMinTime(vec[0], vec[1]);
}

void ShortestPathTask::run()
{
    string src = point;
    map<string, float> currentRoute;
    set<string> currentPoint;
    currentPoint.insert(src);
    for(string dst : *points)
    {
        string od = src + " " + dst;
        if(directMap->at(hour).find(od) != directMap->at(hour).end())
        {
            currentRoute[od] = directMap->at(hour)[od];
        }
    }

    while(1)
    {
        string minDst;
        float minDelay = 999999;
        //找到当前未遍历的最近节点
        for(string dst : *points)
        {
            if(currentPoint.find(dst) != currentPoint.end())
            {
                continue;
            }
            string od = src + " " + dst;
            if(currentRoute.find(od) != currentRoute.end() &&
                    currentRoute[od] < minDelay)
            {
                minDelay = currentRoute[od];
                minDst = dst; 
            }
        }
        if(minDst.size() == 0)
        {
            break;
        }
        currentPoint.insert(minDst);
        currentRoute[src + " " + minDst] = minDelay;
        //更新起点到其他节点的距离
        for(string dst : *points)
        {
            if(currentPoint.find(dst) != currentPoint.end())
            {
                continue;
            }
            string od = src + " " + dst;
            string first = src + " " + minDst;
            string second = minDst + " " + dst;
            int currentHour = hour + minDelay / 60;
            while(currentHour > hour && directMap->find(currentHour) == directMap->end())
            {
                currentHour--;
            }
            if(directMap->at(currentHour).find(second) != directMap->at(currentHour).end()
                    && (currentRoute.find(od) == currentRoute.end() 
                        || currentRoute[od] > minDelay + directMap->at(currentHour)[second]))
            {
                currentRoute[od] = minDelay + directMap->at(currentHour)[second];
            }
        }
    }
    pthread_mutex_lock(&mutex);
    auto ite = currentRoute.begin();
    while(ite != currentRoute.end())
    {
        totalRouteMap->at(hour)[ite->first] = ite->second;
        ite++;
    }
    pthread_mutex_unlock(&mutex);
    cout << point << endl;
}
