#include "Point.h"
#include "common_function.h"
#include<algorithm>
#include<sstream>
using namespace std;
Point::Point()
{
	longitude = latitude = 0.0;
}
Point::Point(float lat, float lng)
{
	this->latitude = lat;
	this->longitude = lng;
}
Point::Point(string lat, string lng)
{
	stringstream ss;
	ss << lat;
	ss >> this->latitude;
	ss.clear();
	ss << lng;
	ss >> this->longitude;
}
Point::Point(const Point& point)
{
	this->latitude = point.latitude;
	this->longitude = point.longitude;
}
Point& Point::operator=(const Point &point)
{
	this->latitude = point.latitude;
	this->longitude = point.longitude;
	return *this;
}

bool Point::operator<(const Point &point)
{
	int lat1 = latitude * 1000;
	int lat2 = point.latitude * 1000;
	int lng1 = longtitude * 1000;
	int lng2 = longtitude * 1000;
	if(lat1 <= lat2)
	{
		return true;
	}
	if(lat1 == lat2 && lng1 <= lng2)
	{
		return true;
	}
	return false;
}
bool Point::operator==(const Point &point)
{
	int lat1 = latitude * 1000;
	int lat2 = point.latitude * 1000;
	int lng1 = longtitude * 1000;
	int lng2 = longtitude * 1000;
	if(lat1 == lat2 && lng1 == lng2)
	{
		return true;
	}
	return false;
}


Point Point::convertToBaidu() const
{
	float bd_lat = 0.0;
	float bd_lng = 0.0;
	float x = this->longitude;
	float y = this->latitude;
	float z = sqrt(x * x + y * y) + 0.00002 * sin(y * M_PI);
	float theta = atan2(y, x) + 0.000003 * cos(x * M_PI);
	//bd_lng = z * cos(theta) + 0.0065;
	//bd_lat = z * sin(theta) + 0.006;
	bd_lng = z * cos(theta) + 0.011;
	bd_lat = z * sin(theta) + 0.004;
	return Point(bd_lat, bd_lng);
}

Point Point::convertToGoogle() const
{
	Point point;
	float bd_x=this->longitude - 0.0065;
	float bd_y=this->latitude - 0.006;
	float z = sqrt(bd_x * bd_x + bd_y * bd_y) - 0.00002 * sin(bd_y * M_PI);
	double theta = atan2(bd_y, bd_x) - 0.000003 * cos(bd_x * M_PI);
	point.longitude = z * cos(theta);
	point.latitude = z * sin(theta);
	return point;
}

float Point::getDistance(const Point &point)
{
	float rlat1 = rad(this->latitude);
	float rlat2 = rad(point.latitude);
	float a = rlat1 - rlat2;
	float b = rad(this->longitude) - rad(point.longitude);
	float s = 2 * asin(sqrt(pow(sin(a/2), 2) + cos(rlat1) * cos(rlat2) * pow(sin(b / 2), 2)));
	s = s * 6378.137;
	return s * 1000;
}
