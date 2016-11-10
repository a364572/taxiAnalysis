#ifndef __POINT_H_
#define __POINT_H_
#include<string>
class Point
{
public:
	float longitude;
	float latitude;


	Point();
	Point(float lat, float lng);
	Point(std::string lat, std::string lng);
	Point(const Point& point);

	Point& operator= (const Point &point);
	bool operator==(const Point &point);
	bool operator< (const Point &point) const;
	Point convertToBaidu() const;
	Point convertToGoogle() const;
	float getDistance(const Point &point);

};
#endif
