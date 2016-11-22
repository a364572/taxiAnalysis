#include<string.h>
#include<unistd.h>
#include<string>
#include<vector>
#include<fstream>
#include<sstream>
#include<iostream>
#include<set>
#include<algorithm>
#include<time.h>
#include<limits.h>
#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<errno.h>
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

string get_line(char *addr, size_t len)
{
    size_t i = 0;
    while(i < len)
    {
        if(addr[i] == '\n')
        {
            return string(addr, addr + i + 1);
        }
        i++;
    }
    return "";
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

float getExcept(vector<int> &vec)
{
    float sum = 0;
    for(auto val : vec)
    {
        sum += val;
    }
    return sum / vec.size();
}

float getExcept2(vector<int> &vec)
{
    float sum = 0;
    for(auto val : vec)
    {
        sum += val * val;
    }
    return sum / vec.size();
}

//读取出租车数据 统计每个时间段内各个车站附近的车辆数目
//void readTaxiData(string filename, map<string, Station>& stationMap)
void readTaxiData(string filename)
{

   // map<Point, string> positionMap;
   // //车站名 --> 时间区间 --> 车辆集合
   // map<string, map<int, set<string>>> zoneMap;
   // auto ite = stationMap.begin();
   // while(ite != stationMap.end())
   // {
   // 	zoneMap[ite->first] = map<int, set<string>>();
   // 	int cnt = (23 - 6) * 3600 / ZONE_INTERVAL;
   // 	int i = 0;
   // 	while(i < cnt)
   // 	{
   // 		zoneMap[ite->first][i++] = set<string>();
   // 	}
   // 	ite++;
   // }

   // //创建线程池
   // PthreadPool pthreadPool;
   // TaxiTask::stationMap = &stationMap;
   // TaxiTask::zoneMap = &zoneMap;

    PthreadPool pthreadPool;

    string preCard;
	Point prePoint;
//    int preZone = -1;
	string line;
	ifstream fi;
	fi.open(filename.data(), ios::in);

    int fd = open(filename.data(), O_RDONLY);
    if(fd < 0)
    {
        return;
    }
    struct stat st;
    fstat(fd, &st);
    size_t total_size = st.st_size;
    size_t begin = 0;
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    size_t max_size = page_size * (1024 * 256);  //每次映射1GB
    string last_line;
    while(begin < total_size)
    {
        size_t map_size = (total_size - begin) > max_size ? max_size : (total_size - begin);
        //偏移量必须是页面大小的整数倍
        off_t offset = begin;
        void *addr = mmap(NULL, map_size, PROT_READ, MAP_PRIVATE, fd, offset);
        if(addr == (void *)-1)
        {
            cout << "mmap调用失败" << strerror(errno) << endl;
            return;
        }

        size_t size_read = 0;
        while(size_read < map_size)
        {
            string line = get_line((char *)addr + size_read, map_size - size_read);
            if(line.size() > 0)
            {
                size_read += line.size();
                if(!last_line.empty())
                {
                    line = last_line + line;
                    last_line = "";
                }
//                auto result = split(line, ',');
//                if(result.size() < 4)
//                {
//                    continue;
//                }
//                struct tm tm;
//                strptime(result[1].c_str(), "%Y-%m-%d %H:%M:%S", &tm);
//                tm.tm_isdst = -1;
//                //跳过过早或过晚的记录
//                if(tm.tm_hour < 6 || tm.tm_hour >= 23)
//                {
//                    continue;
//                }
//                //分别得到 是否载客，坐标点，车牌号
//                int flag = result[0][6] - '0';
//                if(flag)
//                {
//                    continue;
//                }
//                Point point(result[3], result[2]);
//                string cardID = result[0].substr(0, 5);
//                //得到当前记录对应的时间区间
//                int zoneIndex = (tm.tm_hour - 6) * 3600 + tm.tm_min * 60 + tm.tm_sec;
//                zoneIndex = zoneIndex / ZONE_INTERVAL;
//
//                if(preCard == cardID && prePoint == point && preZone == zoneIndex)
//                {
//                    continue;
//                }
//                preCard = cardID;
//                prePoint = point;
//                preZone = zoneIndex;
//
//                AbstractTask* task = new TaxiTask(point, cardID, zoneIndex);
//                pthreadPool.submitTask(task);
                  MapTask* task = new MapTask(line);
                  pthreadPool.submitTask(task);
                  
            }
            else
            {
                last_line = string((char *)addr + size_read, (char *)addr + map_size);
                break;
            }
        }
        begin += map_size;
        munmap(addr, map_size);
    }
    close(fd);
    pthreadPool.waitForFinish();


}
