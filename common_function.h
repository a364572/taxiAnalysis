#ifndef __COMMON_FUNCTION_H_
#define __COMMON_FUNCTION_H_
#include<string>
#include<vector>
#include<map>

#include "Station.h"

#define MAX_DISTANCE 500     //最大距离500m
#define ZONE_INTERVAL 600       //时间区间10分钟

//读取一行 返回成功读取的字节数
std::string get_line(int fd);
//分割字符串 将每个字符的地址放在dst中
std::vector<std::string> split(std::string str, char delim);

//读取地铁站坐标
std::map<std::string, Station> readStationPos(std::string name);

float rad(float value);
void readTaxiData(std::string, std::map<std::string, Station>&);
#endif
