#ifndef __UTILS_H
#define __UTILS_H
#include<string>
#include<map>
#include<set>
#include<iostream>
#include<vector>
#include<sys/time.h>

#define SERVER_IP "0.0.0.0"
#define SERVER_PORT 9999
#define PARCEL_ID_LENGTH 6
#define EMULATION
#define EMULATION_SPEED 60
#define EMUL_START_HOUR 7
#define EMUL_END_HOUR 18
enum MESSAGE_TYPE
{
    MESSAGE_REGISTER = '1',
    MESSAGE_GET_ROUTE_PARCEL,
    MESSAGE_GET_DELIVER_PARCEL,
    MESSAGE_GET_USER_INFORMATION,
    MESSAGE_ACCEPT_PARCEL,
    MESSAGE_DELIVER_PARCEL,
    MESSAGE_CHANGE_ROUTE,
};

class User
{
public:
    User()
    {
        name = email = telephone = gender = "";
        current_parcel_id = "0";
    }
    std::string name;
    std::string email;
    std::string telephone;
    std::string gender;
    std::string src;
    std::string dst;
    std::string current_parcel_id;
};

class Parcel
{
public:
    Parcel();
    std::string generateID();

    std::string id;
    std::string src;
    std::string dst;
    bool isDelivered;
    bool isCarried;
    std::vector<time_t> times;
    std::vector<std::string> stations;
    time_t start_time;
    time_t end_time;
    
};

class Point;
//用户的邮箱作为id
extern std::map<std::string, User*> userMap;
extern std::map<std::string, Parcel*> parcelMap;
//extern std::map<std::string, std::set<Parcel*> > parcelInLine;
extern std::map<std::string, std::set<Parcel*> > parcelInStation;
extern std::map<std::string, int> stationStrToInt;
extern std::map<int, std::string> stationIntToStr;

void handle_message_in(int fd);
void handle_register(int, std::map<std::string, std::string>&);
void handle_get_route_parcel(int, std::map<std::string, std::string>&);
void handle_get_deliver_parcel(int, std::map<std::string, std::string>&);
void handle_get_user_information(int, std::map<std::string, std::string>&);
void handle_accept_parcel(int, std::map<std::string, std::string>&);
void handle_deliver_parcel(int, std::map<std::string, std::string>&);
void handle_change_route(int, std::map<std::string, std::string>&);

void get_parcel_count(std::string, std::string, std::string&, int&);
void add_parcel(Parcel *, int);
void read_users(std::string);
void read_parcels(std::string);
void write_users(std::string);
void write_parcels(std::string);
int init_matlab();
int is_worth_delivering(int sid, int did, int placeHour, int courDid, int courHour);
void exit_matlab();
template<class T>
std::string toString(T t);

#endif
