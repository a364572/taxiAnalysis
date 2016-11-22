#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string>
#include<map>
#include<iostream>
#include<sstream>
#include<dirent.h>

#include "Station.h"
#include "common_function.h"
#include "TaxiTask.h"

using namespace std;

int main(int argc, char **args)
{
	auto stationMap = readStationPos("../../taxi/地铁站坐标");
	auto ite = stationMap.begin();
	while(ite != stationMap.end())
	{
//		cout<<ite->first<<" " << ite->second.position.latitude << " " << ite->second.position.longitude<<endl;
		ite++;
	}

    map<string, map<int, vector<int>>> zoneMap;
    MapTask::zoneMap = &zoneMap;
    struct dirent *dirent;
    DIR *dir = opendir("../../taxi");
    while((dirent = readdir(dir)))
    {
        string filename = dirent->d_name;
        if(filename.find("201504") == 0)
        {
            filename = "../../taxi/" + filename;
            cout << filename << endl;
	        readTaxiData(filename);
        }
    }
	auto zoneIte = zoneMap.begin();
	while(zoneIte != zoneMap.end())
	{
		string out = zoneIte->first;
		auto zoneIIte = zoneIte->second.begin();
		while(zoneIIte != zoneIte->second.end())
		{
            string str1, str2;
            stringstream ss1, ss2;
            ss1 << getExcept(zoneIIte->second);
            ss1 >> str1;
            ss2 << getExcept2(zoneIIte->second);
            ss2 >> str2;
            string str = to_string(zoneIIte->first + 6) + "_" + 
                to_string(zoneIIte->second.size()) + "_" + str1 + "_" + str2;
            out += " " + str;
			zoneIIte++;
		}
		zoneIte++;
        cout << out << endl;
	}
	return 0;
}
