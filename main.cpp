#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string>
#include<map>
#include<iostream>

#include "Station.h"
#include "common_function.h"

using namespace std;

int main()
{
	auto stationMap = readStationPos("../taxi/地铁站坐标");
	auto ite = stationMap.begin();
	while(ite != stationMap.end())
	{
		cout<<ite->first<<" " << ite->second.position.latitude << " " << ite->second.position.longitude<<endl;
		ite++;
	}
	readTaxiData("../taxi/20150415", stationMap);
	return 0;
}
