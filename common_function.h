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
#define MIN_LATITUDE 30700
#define MAX_LATITUDE 31400
#define MIN_LONGITUDE 121000
#define MAX_LONGITUDE 121950

//读取一行 返回成功读取的字节数
std::string get_line(int fd);
//分割字符串 将每个字符的地址放在dst中
std::vector<std::string> split(std::string str, char delim);

//读取地铁站坐标
std::map<std::string, Station> readStationPos(std::string name);

float rad(float value);
//void readTaxiData(std::string, std::map<std::string, Station>&);
void readTaxiData(std::string); 
void mergeResult();
void readFilterRoad(std::set<std::string> &);

float getExcept(std::vector<int> &);
float getExcept2(std::vector<int> &);

class Record
{
public:
    int cnt;
    double expect;
    double expect2;
    Record() : cnt(0), expect(0), expect2(0)
    {}
};
#endif
