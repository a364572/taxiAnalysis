#ifndef __COMMON_FUNCTION_H_
#define __COMMON_FUNCTION_H_
#include<string>
#include<vector>
#include<map>
#include<set>

#include "Station.h"

#define MAX_DISTANCE 300     //最大距离300m
#define ZONE_INTERVAL 3600       //时间区间60分钟
#define GRANULARITY 10          //地图方块边长
#define MIN_OD_TIME 600            //OD最小耗时 10分钟
#define MIN_OD_DISTANCE 5000
#define MAX_OD_DISTANCE 5000
#define MIN_OD_SPEED 3.0 
#define MIN_HOUR 6
#define MAX_HOUR 7
#define MIN_LATITUDE 30900
//#define MIN_LATITUDE 30700
#define MAX_LATITUDE 31400
#define MIN_LONGITUDE 121300
//#define MIN_LONGITUDE 121000
#define MAX_LONGITUDE 121900
//#define MAX_LONGITUDE 121950
//出租车每分钟1.5元 步行速度1m/s 进出站时间 2分钟
#define TAXI_PRICE 1.5
#define WALK_SPEED 1.0
#define WALK_TIME 2.0
#define METHOD_PRICE 1
#define METHOD_TIME 0

class Record
{
public:
    int cnt;
    double expect;
    double expect2;
    Record() : cnt(0), expect(0), expect2(0)
    {}
};

class TripTime
{
public:
    float min;
    float avg;
    float max;
    float dif;
};
struct Result
{
    int timeIndex;
    int priceIndex;
    float timeDelay;
    float priceDelay;
    float taxiDelay;
    float timeDrive;
    float priceDrive;
    float taxiDrive;
    Result() : timeIndex(0), priceIndex(0), timeDelay(0), priceDelay(0),
        taxiDelay(0), timeDrive(0), priceDrive(0), taxiDrive(0)
    {}
};
struct Info
{
    Info() : index(0), avg(-1), dif(-1), price(0) {}
    int index;
    float avg;
    float dif; 
    float price;
    Info& operator= (Info &info)
    {
        this->avg = info.avg;
        this->dif = info.dif;
        this->price = info.price;
        this->index = info.index;
        return *this;
    }
};
//读取一行 返回成功读取的字节数
std::string get_line(int fd);
//分割字符串 将每个字符的地址放在dst中
std::vector<std::string> split(std::string str, char delim);

//读取地铁站坐标
std::map<std::string, Station> readStationPos(std::string name);

float rad(float value);
void readTaxiData(std::string); 
void mergeResult();
void readFilterRoad(std::set<std::string> &);

float getExcept(std::vector<int> &);
float getExcept2(std::vector<int> &);
int getCount(std::vector<int> &);
std::string get_min_mid_max(std::vector<int> &);
void adjustCarNumber(std::map<std::string, std::map<int, std::set<std::string>>>&, std::map<std::string, std::map<int, Info>>&);
bool checkPosition(std::string pos);
void buildRoute(std::string file);

#endif
