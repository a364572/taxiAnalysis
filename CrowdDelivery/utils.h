#ifndef __UTILS_H
#define __UTILS_H
#include<string>
#include<map>
#include<set>
#include<iostream>
#include<vector>
#include<sys/time.h>

#define PARCEL_ID_LENGTH 10
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
    time_t start_time;
    time_t end_time;
    
};

//用户的邮箱作为id
extern std::map<std::string, User*> userMap;
extern std::map<std::string, Parcel*> parcelMap;
extern std::map<std::string, std::set<Parcel*> > parcelInLine;
extern std::set<std::string> stationSet;

void handle_message_in(int fd);
std::ostream& log();
void handle_register(int, std::map<std::string, std::string>&);
void handle_get_route_parcel(int, std::map<std::string, std::string>&);
void handle_get_deliver_parcel(int, std::map<std::string, std::string>&);
void handle_get_user_information(int, std::map<std::string, std::string>&);
void handle_accept_parcel(int, std::map<std::string, std::string>&);
void handle_deliver_parcel(int, std::map<std::string, std::string>&);
void handle_change_route(int, std::map<std::string, std::string>&);

std::map<std::string, std::string> decodeRequese(std::string);
std::vector<std::string> split(std::string &line, std::string delim);

void add_parcel(Parcel *);
void read_station(std::string);
#endif
