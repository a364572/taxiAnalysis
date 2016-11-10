#include<string.h>
#include<unistd.h>
#include<string>
#include<vector>
#include<fstream>
#include<sstream>
#include<set>
#include<algorithm>
#include<time.h>
#include<limits.h>
#include<stdio.h>
#include "common_function.h"
#include "PthreadPool.h"
#include "TaxiTask.h"

using namespace std;
//读取一行 返回成功读取的字节数
string get_line(int fd)
{
	string line;
	if(fd < 0 ) {
		return line;
	}

	int res = 0;
	char ch;
	res = read(fd, &ch, 1);
	while(res > 0 && ch != '\n') {
		line.push_back(ch);
		res = read(fd, &ch, 1);
	}
	return line;
}

//分割字符串 将每个字符的地址放在dst中
vector<string> split(string str, char delim)
{
	vector<string> res;
	if(str.size() == 0) {
		return res;
	}
	decltype(str.size()) prev = 0;
	decltype(str.size()) index = 0;
	while(index < str.size()) {
		if(str[index] == delim) {
			if(prev != index) {
				res.push_back(str.substr(prev, index - prev));
			}
			prev = index + 1;
		}
		index++;
	}
	if(prev < str.size()) {
		res.push_back(str.substr(prev));
	}
	return res;
}

//读取地铁站坐标
map<string, Station> readStationPos(string name)
{
	map<string, Station> map;
	ifstream fi;
	stringstream ss;
	string line;

	fi.open(name.data(), ios::in);
	while(getline(fi, line)) {
		vector<string> res = split(line, ':');
		vector<string> position = split(res[1], ',');
		Station station;
		station.name = res[0];
		ss.clear();
		ss << position[1];
		ss >> station.position.longitude;
		ss.clear();
		ss << position[0];
		ss >> station.position.latitude;
		map[station.name] = station;
	}
	fi.close();
	return map;
}

float rad(float value)
{
	return value * M_PI / 180;
}

//读取出租车数据 统计每个时间段内各个车站附近的车辆数目
void readTaxiData(string filename, map<string, Station>& stationMap)
{

    map<Point, string> positionMap;
	//车站名 --> 时间区间 --> 车辆集合
	map<string, map<int, set<string>>> zoneMap;
	auto ite = stationMap.begin();
	while(ite != stationMap.end())
	{
		zoneMap[ite->first] = map<int, set<string>>();
		int cnt = (23 - 6) * 3600 / ZONE_INTERVAL;
		int i = 0;
		while(i < cnt)
		{
			zoneMap[ite->first][i++] = set<string>();
		}
		ite++;
	}

    //创建线程池
    PthreadPool pthreadPool;
    TaxiTask::stationMap = &stationMap;
    TaxiTask::zoneMap = &zoneMap;

    string preCard;
	Point prePoint;
    int preZone = -1;
	string line;
	ifstream fi;
	fi.open(filename.data(), ios::in);
	while(getline(fi, line))
	{
		auto result = split(line, ',');
		if(result.size() < 4)
		{
			continue;
		}
		struct tm tm;
		strptime(result[1].c_str(), "%Y-%m-%d %H:%M:%S", &tm);
		tm.tm_isdst = -1;
		//跳过过早或过晚的记录
		if(tm.tm_hour < 6 || tm.tm_hour >= 23)
		{
			continue;
		}
		//分别得到 是否载客，坐标点，车牌号
	//	int flag = result[0][6] - '0';
		Point point(result[3], result[2]);
		string cardID = result[0].substr(0, 5);
		//得到当前记录对应的时间区间
		int zoneIndex = (tm.tm_hour - 6) * 3600 + tm.tm_min * 60 + tm.tm_sec;
		zoneIndex = zoneIndex / ZONE_INTERVAL;

		if(preCard == cardID && prePoint == point && preZone == zoneIndex)
		{
			continue;
		}
        preCard = cardID;
        prePoint = point;
        preZone = zoneIndex;

        AbstractTask* task = new TaxiTask(point, cardID, zoneIndex);
        pthreadPool.submitTask(task);
	}

	auto zoneIte = zoneMap.begin();
	while(zoneIte != zoneMap.end())
	{
		string out = zoneIte->first;
		auto zoneIIte = zoneIte->second.begin();
		while(zoneIIte != zoneIte->second.end())
		{
			out = out + " " + to_string(zoneIIte->second.size());
			zoneIIte++;
		}
		zoneIte++;
		printf("%s\n", out.c_str());
	}

}
