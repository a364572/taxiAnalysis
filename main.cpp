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

void handleRouteDecision(string);
    
int main(int argc, char **args)
{
    if(argc == 2)
    {
        //mergeResult();
        //handleRouteDecision(args[1]);
        buildRoute("result_min/expect6-21");
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

            float avg1 = strtof(res[2].data(), NULL);
            float avg2 = strtof(res[3].data(), NULL);
            TripTime time;
            time.min = strtof(res[4].data(), NULL);
            time.avg = avg1;
            time.max = strtof(res[6].data(), NULL);
            time.dif = avg2 - avg1 * avg1;
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


void handleRouteDecision(string argc)
{
    //读取地铁站坐标
	auto stationMap = readStationPos("../../taxi/地铁站坐标");
    //读取地铁运行时间信息
    auto subwayMap = readSubwayTime();
    //计算各个区域的平均等车时间
    auto waitTaxiMap = readWaitTaxiTime();
    //读取区域之间的车辆行驶时间
    auto taxiODMap = readTaxiTime("result_min/expect6-21");
    //读取地铁票价信息
    auto priceMap = readSubwayPrice();
    
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
    ResultTask::subwayPriceMap = &priceMap;
    ResultTask::fd = fd;

    PthreadPool pthreadPool;

    set<string> set;
    ifstream file;
    file.open(("experiment/experiment_tmp" + argc).data(), ios::in);
    if(file.is_open())
    {
        string line;
        while(getline(file, line))
        {
            auto vec = split(line, ' ');
            string od = vec[0] + " " + vec[1];
            set.insert(od);
        }
    }
    file.close();

    ifstream in;
    in.open(("result_min/more" + argc).data(), ios::in);
    if(!in.is_open())
    {
        return;
    }
        
    for(float p = 0.8; p <= 0.81; p+= 0.05)
    {
        string line;
        while(getline(in, line))
        {
            auto vec = split(line, ' ');
            string od = vec[0] + " " + vec[1];
            if(set.find(od) != set.end())
            {
                continue;
            }
            ResultTask *task = new ResultTask(line);
            pthreadPool.submitTask(task);
        }
    }
    in.close();
    pthreadPool.waitForFinish();
    close(fd);
}
