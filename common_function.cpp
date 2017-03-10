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
#include<dirent.h>
#include<stdlib.h>
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
        int lat = station.position.latitude * 1000;
        int lng = station.position.longitude * 1000;
        lat = lat - lat % GRANULARITY;
        lng = lng - lng % GRANULARITY;
        station.positionString = to_string(lat) + "," + to_string(lng);
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
    decltype(vec.size()) filter = 0;
    if(vec.size() >= 100)
    {
        filter = vec.size() * 0.4;
    }

    for(auto i = filter / 2; i < vec.size() - filter / 2; i++)
    {
        sum += vec[i];
    }
    return sum / (vec.size() - filter);
}

float getExcept2(vector<int> &vec)
{
    float sum = 0;
    decltype(vec.size()) filter = 0;
    if(vec.size() >= 100)
    {
        filter = vec.size() * 0.4;
    }

    for(auto i = filter / 2; i < vec.size() - filter / 2; i++)
    {
        sum += vec[i] * vec[i];
    }
    return sum / (vec.size() - filter);
}

int getCount(vector<int> &vec)
{
    int filter = 0;
    if(vec.size() >= 100)
    {
        filter = vec.size() * 0.4;
    }
    return vec.size() - filter;
}

string get_min_mid_max(vector<int> &vec)
{
    int min = 0, max = vec.size() - 1;
    if(vec.size() >= 100)
    {
        min = vec.size() * 0.2;
        max = vec.size() - vec.size() * 0.2;
    }
    int mid = (min + max) / 2;
    return to_string(vec[min]) + "_" + to_string(vec[mid]) + "_"
        + to_string(vec[max]);
}

//读取出租车数据 统计每个时间段内各个车站附近的车辆数目
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
                TimeTask* task = new TimeTask(line);
                //MapTask* task = new MapTask(line);
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

void mergeResult()
{
    map<string, map<int, Record>> recordMap; 

    DIR* dir = opendir("car_number");
    struct dirent* dirent;
    while((dirent = readdir(dir)) != NULL)
    {
        string filename = dirent->d_name;
        if(filename.find_first_of(".") == 0)
        {
            continue;
        }
        filename = "car_number/" + filename;
        ifstream fd;
        fd.open(filename.data(), ios::in);
        if(!fd.is_open())
        {
            cout << filename << "打开失败" << endl;
            continue;
        }
        string line;
        while(getline(fd, line))
        {
            auto result = split(line, ' ');
            auto key = result[0];
            if(recordMap.find(key) == recordMap.end())
            {
                recordMap[key] = map<int, Record>();
            }
            auto ite = result.begin();
            ite++;
            while(ite != result.end())
            {
                auto res = split(*ite, '_');
                int index = atoi(res[0].data());
                double avg = strtod(res[1].data(), NULL);
                double dif = strtod(res[2].data(), NULL);
                if(recordMap[key].find(index) == recordMap[key].end())
                {
                    recordMap[key][index] = Record();
                }
                if (avg < 0)
                {
                    avg = 61;
                    dif = 3721;
                }
                recordMap[key][index].expect += avg;
                recordMap[key][index].expect2 += dif;
                recordMap[key][index].cnt++;
                ite++;
            }
        }
        fd.close();
    }
    auto ite = recordMap.begin();
    while(ite != recordMap.end())
    {
        string out = ite->first;
        auto iite = ite->second.begin();
        while(iite != ite->second.end())
        {
            out += " " + to_string(iite->first);
            out += "_" + to_string(iite->second.expect / iite->second.cnt);
            out += "_" + to_string(iite->second.expect2 / iite->second.cnt);
            iite++;
        }
        cout << out << endl;
        ite++;
    }
}

void readFilterRoad(set<string> &filter)
{
    ifstream fd;
    fd.open("../../taxi/道路信息.txt", ios::in);
    string line;
    getline(fd, line);
    while(getline(fd, line))
    {
        auto res = split(line, ',');
        if(res[6] == "motorway" || res[6] == "motorway_link")
        {
            filter.insert(res[0]);
        }
    }
}
void adjustCarNumber(map<string, map<int, set<string>>>& carMap, map<string, map<int, Info>>& result)
{
    auto ite = carMap.begin();
    while(ite != carMap.end())
    {
        string zone = ite->first;
        vector<string> vec = split(zone, ',');
        if(vec.size() != 2)
        {
            continue;
        }
        int latitude = atoi(vec[0].data());
        int longitude = atoi(vec[1].data());

        result[zone] = map<int, Info>();
        auto iite = ite->second.begin();
        while(iite != ite->second.end())
        {
            int time = iite->first;
            if(time < MIN_HOUR || time >= MAX_HOUR)
            {
                iite++;
                continue;
            }
            float localNumber = iite->second.size();
            float localWait = 61;
            set<string> cars(iite->second);

            for(int i = -GRANULARITY; i <= GRANULARITY; i += GRANULARITY)
            {
                int lat = latitude + GRANULARITY;
                for(int j = -GRANULARITY; j <= GRANULARITY; j += GRANULARITY)
                {
                    int lng = longitude + GRANULARITY;
                    string region = (to_string(lat) + "," + to_string(lng));
                    if(region != zone && carMap.find(region) != carMap.end()
                            && carMap[region].find(time) != carMap[region].end())
                    {
                        set<string> un;
                        set_union(cars.begin(), cars.end(),
                                carMap[region][time].begin(), carMap[region][time].end(),
                                inserter(un, un.begin()));
                        int dif = un.size() - cars.size(); 
                        float drive_time = 2;
                        float tmp = drive_time * dif + 60;
                        localNumber = (60 * dif + localNumber * tmp) / tmp;
                        localWait = 60 / localNumber; 
                        cars.swap(un);
                    }
                }
            }
            Info info;
            info.avg = localWait;
            info.dif = info.avg * info.avg;
            result[zone][time] = info;
            iite++;
        }
        ite++;
    }
}
bool checkPosition(string pos)
{
    auto vec = split(pos, ',');
    int lat = atoi(vec[0].data());
    int lng = atoi(vec[1].data());
    if(lat < 31110 || lat > 31340 || lng < 121200 || lng > 121800)
    {
        return true;
    }
    return false;
}

//<时间-><OD->信息>>
void buildRoute(string file)
{
    ifstream in;
    in.open(file.data(), ios::in);
    if(!in.is_open())
    {
        cout << "文件不存在" << endl;
        return;
    }

    set<string> points;
    map<int, map<string, float>> totalRoute;
    map<int, map<string, float>> totalDif;
    map<int, map<string, float>> direct;
    map<int, map<string, float>> directDif;
    for(int i = MIN_HOUR; i < MAX_HOUR; i++)
    {
        totalRoute[i] = map<string, float>();
        totalDif[i] = map<string, float>();
        direct[i] = map<string, float>();
        directDif[i] = map<string, float>();
    }

    //读取历史数据
    int cnt = 0;
    string line;
    while(getline(in, line))
    {
        auto vec = split(line, ' ');
        points.insert(vec[0]);
        points.insert(vec[1]);
        string od = vec[0] + " " + vec[1];
        auto position = split(vec[0], ',');
        string s_lat = position[0].substr(0, 2) + "." + position[0].substr(2);
        string s_lng = position[1].substr(0, 3) + "." + position[1].substr(3);
        position = split(vec[1], ',');
        string e_lat = position[0].substr(0, 2) + "." + position[0].substr(2);
        string e_lng = position[1].substr(0, 3) + "." + position[1].substr(3);
        Point start(s_lat, s_lng);
        Point end(e_lat, e_lng);
        float distance = start.getDistance(end);
        for(decltype(vec.size()) i = 2; i < vec.size(); i++)
        {
            auto tmp = split(vec[i], '_');
            int hour = atoi(tmp[0].data());
            float avg = strtof(tmp[2].data(), NULL);
            float avg2 = strtof(tmp[3].data(), NULL);

            if( avg < 0.1 || distance / avg / 60 > 30)
            {
                cnt++;
                //cout << start.latitude <<" " << start.longitude<< " " << end.latitude << " "
                //    << end.longitude << " " << distance << " " << avg << endl;
                continue;
            }
            directDif[hour][od] = avg2 - avg * avg;
            if(avg < 2)
            {
                avg = 2;
            }
            direct[hour][od] = avg;
        }
    }
    in.close();

    cout << cnt << endl;

    ShortestPathTask::points = &points;
    ShortestPathTask::totalRouteMap = &totalRoute;
    ShortestPathTask::totalDifMap = &totalDif;
    ShortestPathTask::directMap = &direct;
    ShortestPathTask::directDifMap = &directDif;

    PthreadPool pthreadPool;
    for(int hour = MIN_HOUR; hour < MAX_HOUR; hour++)
    {
        //对每个小时进行遍历
        for(string src : points)
        {
            ShortestPathTask* task = new ShortestPathTask(hour, src);
            pthreadPool.submitTask(task);
        }
    }
    pthreadPool.waitForFinish();

    for(auto src : points)
    {
        for(auto dst : points)
        {
            string od = src + " " + dst;
            string out = od;
            for(int i = MIN_HOUR; i < MAX_HOUR; i++)
            {
                if(totalRoute[i].find(od) != totalRoute[i].end())
                {
                    out = out + " " + to_string(i) + "_" + to_string(totalRoute[i][od]);
                }
                if(totalDif[i].find(od) != totalDif[i].end())
                {
                    out = out + "_" + to_string(totalDif[i][od]);
                }
            }
            if(od.size() < out.size())
            {
                cout << out << endl;
            }
        }
    }

}
