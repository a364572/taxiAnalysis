#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<iostream>
#include<fstream>
#include<sstream>
#include<algorithm>
#include<dirent.h>
#include<climits>
#include<string.h>
#include<unistd.h>

#include "Station.h"
#include "common_function.h"
#include "TaxiTask.h"
#include "PthreadPool.h"

using namespace std;

struct TestOD;
void handleRouteDecision(string);
void get_od_count(map<string, vector<int>>&, string);
void readTestOD(string);
    
int main(int argc, char **args)
{
    if(argc == 2)
    {
        //mergeResult();
        handleRouteDecision(args[1]);
        //buildRoute("result_min/expect6-21");
        //readTestOD("/home/vlab/tmp/taxi/20150430");
        //
        //map<string, vector<int>> result;
        //int day = 24;
        //while(day <= 30)
        //{
        //    string file = "/home/vlab/tmp/taxi/201504" + to_string(day);
        //    get_od_count(result, file);
        //    day++;
        //}
        //auto ite = result.begin();
        //while(ite != result.end())
        //{
        //    sort(ite->second.begin(), ite->second.end());
        //    float avg = getExcept(ite->second);
        //    float avg2 = getExcept2(ite->second);
        //    int cnt = ite->second.size();
        //    cout << ite->first << " " << avg << " " << avg2 << " " << cnt << endl;
        //    ite++;
        //}
        return 0;
    }
    //	auto stationMap = readStationPos("../../taxi/地铁站坐标");
    //	auto ite = stationMap.begin();
    //	while(ite != stationMap.end())
    //	{
    //		cout<<ite->first<<" " << ite->second.position.latitude << " " << ite->second.position.longitude<<endl;
    //		ite++;
    //	}
    set<string> filter;
    readFilterRoad(filter);
    MapTask::filterSet= &filter;
    TimeTask::filterSet = &filter;

    map<string, map<int, vector<int>>> zoneMap;
    map<string, map<int, set<string>>> zoneMap1;
    MapTask::zoneMap = &zoneMap;
    TimeTask::zoneMap = &zoneMap1;

    struct dirent *dirent;
    DIR *dir = opendir("../../taxi");
    while((dirent = readdir(dir)))
    {
        string filename = dirent->d_name;
        if(filename.find("201504") == 0)
        {
            string name = "car_number/res" + filename;
            filename = "../../taxi/" + filename;
            cout << filename << endl;
	        readTaxiData(filename);

            //*****insert here
            map<string, map<int, Info>> result;
            adjustCarNumber(zoneMap1, result);
            cout << zoneMap1.size() << " " << result.begin()->second.size() << endl;
            ofstream fd;
            fd.open(name.c_str(), ios::out);
            
            auto zoneIte = result.begin();
            while(zoneIte != result.end())
            {
                string out = zoneIte->first;
                auto& arr = zoneIte->second;
                for(int i = MIN_HOUR; i < MAX_HOUR; i++)
                {
                    out = out + " " + to_string(i);
                    if(arr.find(i) == arr.end())
                    {
                        out += "_61_3600";
                    }
                    else
                    {
                        out += "_" + to_string((int)arr[i].avg) + "_" + to_string((int)arr[i].dif);
                    }
                }
                fd << out << endl;
                zoneIte++;
            }

            map<string, map<int, set<string>>> tmp;
            zoneMap1.swap(tmp);
            zoneMap1.clear();
            fd.close();
        }
    }
//    auto zoneIte = zoneMap.begin();
//    while(zoneIte != zoneMap.end())
//    {
//        string out = zoneIte->first;
//        auto zoneIIte = zoneIte->second.begin();
//        while(zoneIIte != zoneIte->second.end())
//        {
//            string str1, str2;
//            stringstream ss1, ss2;
//
//            sort(zoneIIte->second.begin(), zoneIIte->second.end());
//            char tmp[64];
//            int cnt = getCount(zoneIIte->second);
//            double expect = getExcept(zoneIIte->second);
//            double expect2 = getExcept2(zoneIIte->second);
//            memset(tmp, 0, sizeof(tmp));
//            sprintf(tmp, " %d_%d_%lf_%lf_", (zoneIIte->first + 6), cnt, expect, expect2);
//            out += tmp + get_min_mid_max(zoneIIte->second);
//            zoneIIte++;
//        }
//        zoneIte++;
//        cout << out << endl;
//    }

    //    mergeResult();
    return 0;
}

map<string, map<int, Info>> readWaitTaxiTime()
{
    map<string, map<int, Info>> result;
    ifstream fd;
    fd.open("car_number/等车时间", ios::in);
    if(!fd.is_open())
    {
        cout << "打开失败" << endl;
        return result;
    }
    string line;
    while(getline(fd, line))
    {
        auto vec = split(line, ' ');
        string zone = vec[0];
        result[zone] = map<int, Info>();
        for(decltype(vec.size()) i = 1; i < vec.size(); i++)
        {
            auto res = split(vec[i], '_');
            int hour = atoi(res[0].data());
            if(hour < MIN_HOUR)
            {
                continue;
            }
            Info info;
            info.avg = strtof(res[1].data(), NULL);
            info.dif = info.avg * info.avg;
            result[zone][hour] = info;
        }
    } 
    return result;
}
map<string, map<int, Info>> readSubwayTime()
{
    map<string, map<int, Info>> odMap;
    ifstream in;
    in.open("/home/vlab/workplace/experiment/routeInfo/1.0", ios::in);
    if(!in.is_open())
    {
        cout << "打开文件失败" << endl;
        return odMap;
    }
    string line;
    string preOD = "";
    int preHour = 0; int totalCnt = 0; double totalAVG1 = 0;
    double totalAVG2 = 0;;
    while(getline(in, line))
    {
        vector<string> vec = split(line, ' ');
        string od = vec[0] + " " + vec[1];
        stringstream ss1, ss2, ss3;
        int hour = atoi(vec[2].substr(0, 2).data());
        int cnt = atoi(vec[9].data());
        float avg1 = strtof(vec[7].data(), NULL);
        float avg2 = strtof(vec[8].data(), NULL);
        if(preOD == od && preHour == hour && hour >= MIN_HOUR && hour < MAX_HOUR)
        {
            totalCnt += cnt;
            totalAVG1 += avg1 * cnt;
            totalAVG2 += avg2 * cnt;
        }
        else if(preOD != "")
        {
            if(odMap.find(preOD) == odMap.end())
            {
                odMap[preOD] = map<int, Info>();
            }
            if(preHour >= MIN_HOUR && preHour < MAX_HOUR)
            {
                Info info;
                info.avg = totalAVG1 / totalCnt;
                info.dif = totalAVG2 / totalCnt - info.avg * info.avg;
                odMap[preOD][preHour] = info;
                totalCnt = cnt;
                totalAVG1 = avg1 * cnt;
                totalAVG2 = avg2 * cnt;
            }
        }
        preOD = od;
        preHour = hour;
    }
    in.close();

    if(odMap.find(preOD) == odMap.end())
    {
        odMap[preOD] = map<int, Info>();
    }
    if(preHour >= MIN_HOUR && preHour < MAX_HOUR)
    {
        Info info;
        info.avg = totalAVG1 / totalCnt;
        info.dif = totalAVG2 / totalCnt - info.avg * info.avg;
        odMap[preOD][preHour] = info;
    }
    return odMap;
}

map<string, map<int, TripTime>> readTaxiTime(string file)
{
    map<string, map<int, TripTime>> result;
    ifstream in;
    in.open(file.data(), ios::in);
    if(!in.is_open())
    {
        cout << "文件打开失败" << endl;
        return result;
    }
    string line;
    while(getline(in, line))
    {
        vector<string> vec = split(line, ' ');
        if(vec.size() < 2)
        {
            continue;
        }
        string od = vec[0] + ' ' + vec[1];
        result[od] = map<int, TripTime>();
        for(decltype(vec.size()) i = 2; i < vec.size(); i++)
        {
            auto res = split(vec[i], '_');
            int hour = atoi(res[0].data()); 
            if(hour < MIN_HOUR)
            {
                continue;
            }

            //float avg1 = strtof(res[2].data(), NULL);
            //float avg2 = strtof(res[3].data(), NULL);
            //TripTime time;
            //time.min = strtof(res[4].data(), NULL);
            //time.avg = avg1;
            //time.max = strtof(res[6].data(), NULL);
            //time.dif = avg2 - avg1 * avg1;
            TripTime time;
            time.avg = strtof(res[1].data(), NULL);
            result[od][hour] = time;
        }
    }
    return result;
}

map<string, int> readSubwayPrice()
{
    map<string, int> result;
    ifstream in;
    in.open("price.txt", ios::in);
    if(!in.is_open())
    {
        cout << "文件打开失败" << endl;
        return result;
    }
    string line;
    while(getline(in, line))
    {
        auto vec = split(line, ' ');
        string od = vec[0] + " " + vec[1];
        result[od] = atoi(vec[2].data());
    }
    in.close();
    return result;
}

float getProbability(Info &info, float expect)
{
    if(info.avg < 0 || info.dif < 0 || info.avg >= expect)
    {
        return -1;
    }
    float diff = expect - info.avg;

    return 1.0 - info.dif / (diff * diff);
}

struct Position
{
    Point pos;
    int second;
    string road;
    Position(string str)
    {
        auto res = split(str, ',');
        second = MapTask::getSecond(res[0]);
        pos = MapTask::getPoint(res[1], res[2]);
        road = res[3];
    }
};
void readTestOD(string file)
{
    vector<TestOD> ods;
    ifstream in;
    in.open(file, ios::in);
    if(!in.is_open())
    {
        cout << "测试OD打开失败" << endl;
    }
    string line;
    while(getline(in, line))
    {
        int flag = 1;
        auto vec = split(line, ' ');
        vector<Position> points;

        if(vec[1] == "0")
        {
            flag = 0;
        }
        auto ite = vec.begin();
        ite++;
        ite++;
        while(ite != vec.end())
        {
            points.push_back(Position(*ite));
            ite++;
        }
        //过早或过晚
        if(points.front().second / 3600 < MIN_HOUR || points.back().second / 3600 >= MAX_HOUR)
        {
            continue;
        }
        //旅程时间大于3小时
        int total_time = points.back().second - points.front().second;
        if(total_time >= 3 * 3600)
        {
            continue;
        }

        float total_distance = 0;
        for(vector<Position>::size_type i = 0; i < points.size(); )
        {

            auto end_i  = i + 1;
            while(end_i < points.size() && points[end_i].pos == points[i].pos)
            {
                end_i++;
            }
            
            //计算一段距离
            float distance = 0;
            if(end_i >= points.size())
            {
                distance = points[end_i - 1].pos.getDistance(points[i].pos);
                total_distance += distance;
            }
            else
            {
                distance = points[end_i].pos.getDistance(points[i].pos);
            }
            total_distance += distance;
            i = end_i;
        }
        int speed = total_distance / total_time;
        if(speed < MIN_OD_SPEED)
        {
            continue;
        }
        TestOD od;
        od.flag = flag;
        od.src = vec[2];
        od.dst = vec.back();
        od.distance = total_distance;
        ods.push_back(od);
    }
    for(auto od : ods)
    {
        cout << od.src << " " << od.dst << " " << od.flag << " " << od.distance << endl;
    }
}

//读取日常出行OD
vector<TestOD> readDailyOD()
{
    vector<TestOD> ods;
    ifstream in;
    in.open("ods", ios::in);
    if(!in.is_open())
    {
        cout << "日常OD读取失败" << endl;
    }
    int cnt = 0;
    string line;
    while(getline(cin, line))
    {
        auto vec = split(line, ' ');
        auto begins = split(vec[0], ',');
        auto ends = split(vec[1], ',');
        int begin_sec = MapTask::getSecond(begins[0]);
        int end_sec = MapTask::getSecond(ends[0]);
        string src = MapTask::get_round(begins[1]) + "," + MapTask::get_round(begins[2]);
        string dst = MapTask::get_round(ends[1]) + "," + MapTask::get_round(ends[2]);
        if(src.size() < 24 || dst.size() < 24)
        {
            continue;
        }
        TestOD od;
        od.src = src;
        od.dst = dst;
        od.begin = begin_sec;
        od.end = end_sec;
        od.flag = stoi(vec[2]);
        od.distance = stoi(vec[3]);
        ods.push_back(od);
        cnt++;
    }
    cout << "共有 " << cnt << " 条测试OD" << endl;
    return ods;
}

//读取区域之间的车辆行驶距离
map<string, int> readTaxiDistance()
{
    map<string, int> res;
    map<string, vector<int>> tmp;
    ifstream in;
    in.open("/home/vlab/tmp/distance_gg", ios::in);
    if(!in.is_open())
    {
        cout << "车辆行驶距离文件打开失败" << endl;
    }
    string line;
    while(getline(in, line))
    {
        auto vec = split(line, ' ');
        auto begins = split(vec[0], ',');
        auto ends = split(vec[1], ',');
        int distance = stoi(vec[2]);

        string src = MapTask::get_round(begins[0]) + "," + MapTask::get_round(begins[1]);
        string dst = MapTask::get_round(ends[0]) + "," + MapTask::get_round(ends[1]);
        string od = src + " " + dst;
        if(tmp.find(od) == tmp.end())
        {
            tmp[od] = vector<int>();
        }
        tmp[od].push_back(distance);
    }
    int cnt = 0;
    auto ite = tmp.begin();
    while(ite != tmp.end())
    {
        int sum = 0;
        auto iite = ite->second.begin();
        while(iite != ite->second.end())
        {
            sum += *iite;
            iite++;
        }
        res[ite->first] = sum / ite->second.size();
        ite++;
        cnt++;
    }
    cout << "共有 " << cnt << " 条行驶时间" << endl;
    return res;
}

void handleRouteDecision(string argc)
{
    //读取地铁站坐标
    cout << "读取地铁站坐标" << endl;
	auto stationMap = readStationPos("../../taxi/地铁站坐标");
    //读取地铁运行时间信息
    cout << "读取地铁运行时间信息" << endl;
    auto subwayMap = readSubwayTime();
    //计算各个区域的平均等车时间
    cout << "计算各个区域的平均等车时间" << endl;
    auto waitTaxiMap = readWaitTaxiTime();
    //读取区域之间的车辆行驶时间
    cout << "读取区域之间的车辆行驶时间" << endl;
    auto taxiODMap = readTaxiTime("result_min/route");
    //读取区域之间的车辆行驶距离
    cout << "读取区域之间的车辆行驶距离" << endl;
    auto taxiODDisMap = readTaxiDistance();
    //读取地铁票价信息
    cout << "读取地铁票价信息" << endl;
    auto priceMap = readSubwayPrice();
    
    //打开测试OD文件
    cout << "打开测试OD文件" << endl;
    auto dailyOD = readDailyOD();
    
    int fd = open(("experiment/experiment" + argc).data(), O_RDWR | O_TRUNC | O_CREAT, 0666);
    if(fd < 0)
    {
        cout << "输出文件打开失败" << endl;
        return;
    }

    ResultTask::stationMap = &stationMap;
    ResultTask::subwayMap = &subwayMap;
    ResultTask::waitTaxiMap = &waitTaxiMap;
    ResultTask::taxiODMap = &taxiODMap;
    ResultTask::taxiDisMap = &taxiODDisMap;
    ResultTask::subwayPriceMap = &priceMap;
    ResultTask::fd = fd;

    set<string> doneSet;
    ifstream file;
    file.open(("experiment/experiment_tmp" + argc).data(), ios::in);
    if(file.is_open())
    {
        string line;
        while(getline(file, line))
        {
            auto vec = split(line, ' ');
            string od = vec[0] + " " + vec[1];
            doneSet.insert(od);
        }
    }
    file.close();


    srand(time(0));
    //random_shuffle(testOD.begin(), testOD.end());

    PthreadPool pthreadPool;

    for(vector<int>::size_type i = 0; i < dailyOD.size(); i++) 
    {
        ResultTask *task = new ResultTask("");
        task->od = dailyOD[i];
        pthreadPool.submitTask(task);
    }

    pthreadPool.waitForFinish();
    close(fd);
}

//统计OD计数
void get_od_count(map<string, vector<int>>& result, string file)
{
    ifstream in;
    in.open(file.data(), ios::in);
    if(!in.is_open())
    {
        cout << "文件打开失败" << endl;
        return;
    }
    string line;
    while(getline(in, line))
    {
        auto vec = split(line, ' ');
        if(vec.size() > 4)
        {            
            auto vals1 = split(vec[2], ',');
            auto vals2 = split(vec[vec.size() - 1], ',');
            string time1 = "2015-04-30 " + vals1[0];
            string time2 = "2015-04-30 " + vals2[0];
            struct tm tm1, tm2;
            time_t t1, t2;
            strptime(time1.data(), "%Y-%m-%d %H:%M:%S", &tm1);
            strptime(time2.data(), "%Y-%m-%d %H:%M:%S", &tm2);
            t1 = mktime(&tm1);
            t2 = mktime(&tm2);

            Point start(vals1[1], vals1[2]);
            Point end(vals2[1], vals2[2]);
            int min = (t2 - t1) / 60 + 1;
            if(tm1.tm_hour < MIN_HOUR || tm2.tm_hour >= MAX_HOUR ||
                    start.getDistance(end) / (t2 - t1) < MIN_OD_SPEED)
            {
                continue;
            }
            int src_lat = stod(vals1[1]) * 1000;
            int src_lng = stod(vals1[2]) * 1000;
            int dst_lat = stod(vals2[1]) * 1000;
            int dst_lng = stod(vals2[2]) * 1000;

            src_lat -= src_lat % (1 * GRANULARITY);
            src_lng -= src_lng % (1 * GRANULARITY);
            dst_lat -= dst_lat % (1 * GRANULARITY);
            dst_lng -= dst_lng % (1 * GRANULARITY);

            string od = to_string(src_lat) + "," + to_string(src_lng) + " " + to_string(dst_lat) + "," + to_string(dst_lng);
            if(result.find(od) == result.end())
            {
                result[od] = vector<int>();
            }
            result[od].push_back(min);
        }            
    }                
}                    
