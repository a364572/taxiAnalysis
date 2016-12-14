#include "utils.h"
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<sys/time.h>
#include<string.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<fstream>
using namespace std;

map<string, User*> userMap;
map<string, Parcel*> parcelMap;
map<string, set<Parcel*> > parcelInLine;
set<string> stationSet;

string Parcel::generateID()
{
    string id = "";
    for(int i = 0; i < PARCEL_ID_LENGTH; i++)
    {
        int index = random() % 26; 
        id.push_back('a' + index);
    }
    return id;
}
Parcel::Parcel()
{
    isDelivered = false;
    src = dst = "";
    start_time = time(NULL);
    id = generateID();
    while(parcelMap.find(id) != parcelMap.end())
    {
        id = generateID();
    }
}

ostream& log()
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char tmp[64];
    memset(tmp, 0, sizeof(tmp));
    sprintf(tmp, "%4d-%02d-%02d %02d:%02d:%02d : ", tm->tm_year + 1900, 
            tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    return cout << tmp;
}

vector<string> split(string& line, string delim)
{
    vector<string> res;
    string::size_type start = 0;
    string::size_type pos = line.find_first_of(delim, start); 
    while(pos != string::npos)
    {
        res.push_back(line.substr(start, pos - start));
        start = pos + delim.size();
        pos = line.find_first_of(delim, start);
    }
    res.push_back(line.substr(start));
    return res;
}
void handle_message_in(int fd)
{
    int message_type = 0;
    if(fd < 0 || recv(fd, &message_type, 1, 0 ) != 1)
    {
        log() << "读取消息出错" << endl;
        return;
    }

    switch(message_type)
    {
    case MESSAGE_REGISTER:
        handle_register(fd);
        break;
    case MESSAGE_GET_ROUTE_PARCEL:
        handle_get_route_parcel(fd);
        break;
    case MESSAGE_GET_DELIVER_PARCEL:
        handle_get_deliver_parcel(fd);
        break;
    case MESSAGE_GET_USER_INFORMATION:
        handle_get_user_information(fd);
        break;
    case MESSAGE_ACCEPT_PARCEL:
        handle_accept_parcel(fd);
        break;
    case MESSAGE_DELIVER_PARCEL:
        handle_deliver_parcel(fd);
        break;
    default:
        log() << "不合法的消息类型" << endl;
        break;
    }
}

vector<string> recv_all_message(int fd)
{
    char tmp[1024];
    int total_length = 0;
    int length = recv(fd, tmp + total_length, sizeof(tmp) - total_length - 1, 0); 
    while(length > 0)
    {
        total_length += length;
        length = recv(fd, tmp + total_length, sizeof(tmp) - total_length, 0);
    }

    if(length < 0)
    {
        return vector<string>();
    }
    tmp[total_length] = '\0';
    string res = tmp;
    return split(res, " ");
}

//用户注册
void handle_register(int fd)
{
    vector<string> res = recv_all_message(fd);
    User* user = new User();
    for(vector<string>::size_type i = 0; i < res.size(); i++)
    {
        vector<string> key_value = split(res[i], "_");
        if(key_value.size() != 2)
        {
            continue;
        }

        if(key_value[0] == "name")
        {
            user->name = key_value[1];
        }
        if(key_value[0] == "email")
        {
            user->email = key_value[1];
        }
        if(key_value[0] == "telephone")
        {
            user->telephone = key_value[1];
        }
        if(key_value[0] == "gender")
        {
            user->gender = key_value[1];
        }
    }
    if(user->email.size() > 0)
    {
        userMap[user->email] = user;
        send(fd, "1", 1, 0);
        log() << " 新注册用户" << "\r\n\t姓名:" << user->name <<
           "\r\n\t性别:" << user->gender << "\r\n邮箱:" << user->email <<
           "\r\n\t电话:" << user->telephone << endl;
        return;
    }
    send(fd, "0", 1, 0);
    delete user;
}

//得到某条路径的包裹数量
void handle_get_route_parcel(int fd)
{
    string response = "0";
    string route;
    vector<string> res = recv_all_message(fd);
    for(vector<string>::size_type i = 0; i < res.size(); i++)
    {
        vector<string> key_value = split(res[i], "_");
        if(key_value.size() != 2)
        {
            continue;
        }

        if(key_value[0] == "route")
        {
            map<string, set<Parcel*> >::iterator ite = parcelInLine.find(key_value[1]);
            if(ite != parcelInLine.end())
            {
                int cnt = 0;
                set<Parcel *>::iterator parcelIte = ite->second.begin();
                while(parcelIte != ite->second.end())
                {
                    if(!(*parcelIte)->isDelivered)
                    {
                        cnt++;
                    }
                }
                char tmp[16];
                memset(tmp, 0, sizeof(tmp));
                sprintf(tmp, "%d", cnt);
                response = tmp;
            }
        }
    }
    log() << "路径 " << route << " 当前有 " << response << " 个包裹" << endl;
    send(fd, response.data(), response.size(), 0);
}

//获取正在派送的包裹
void handle_get_deliver_parcel(int fd)
{
    string email;
    string parcelID = "0";
    vector<string> res = recv_all_message(fd);
    for(vector<string>::size_type i = 0; i < res.size(); i++)
    {
        vector<string> key_value = split(res[i], "_");
        if(key_value.size() != 2)
        {
            continue;
        }

        if(key_value[0] == "email")
        {
            email = key_value[1];
            map<string, User*>::iterator ite = userMap.find(key_value[1]);
            if(ite != userMap.end())
            {
                parcelID = ite->second->current_parcel_id;
            }
        }
    }
    log() << " 用户 " << email << " 正在派送包裹" << parcelID << endl;
    send(fd, parcelID.data(), parcelID.size(), 0);
}

//获取用户的信息
void handle_get_user_information(int fd)
{
    string response = "0";
    vector<string> res = recv_all_message(fd);
    for(vector<string>::size_type i = 0; i < res.size(); i++)
    {
        vector<string> key_value = split(res[i], "_");
        if(key_value.size() != 2)
        {
            continue;
        }

        if(key_value[0] == "email")
        {
            map<string, User*>::iterator ite = userMap.find(key_value[1]);
            if(ite == userMap.end())
            {
                response = "0";
            }
            else
            {
                response = "name_" + ite->second->name;
                response += " email_" + ite->second->email;
                response += " telephone_" + ite->second->telephone;
                response += " parcel_" + ite->second->current_parcel_id;
            }
        }
    }
    send(fd, response.data(), response.size(), 0);
}

//用户选择接收包裹
void handle_accept_parcel(int fd)
{
    string parcelID = "0";
    string userEmail;
    vector<string> res = recv_all_message(fd);
    for(vector<string>::size_type i = 0; i < res.size(); i++)
    {
        vector<string> key_value = split(res[i], "_");
        if(key_value.size() != 2)
        {
            continue;
        }

        if(key_value[0] == "route")
        {
            map<string, set<Parcel*> >::iterator ite = parcelInLine.find(key_value[1]);
            if(ite != parcelInLine.end())
            {
                set<Parcel *>::iterator parcelIte = ite->second.begin();
                while(parcelIte != ite->second.end())
                {
                    if(!(*parcelIte)->isDelivered)
                    {
                        parcelID = (*parcelIte)->id;
                    }
                }
            }
        }

        if(key_value[0] == "email")
        {
            userEmail = key_value[1];
        }
    }
    if(parcelID.size() > 1)
    {
        log() << "包裹 " << parcelID << " 被指派给用户" << userEmail << endl;
    }
    send(fd, parcelID.data(), parcelID.size(), 0);
}


//用户送达包裹
void handle_deliver_parcel(int fd)
{
    string parcelID;
    string userEmail;
    vector<string> res = recv_all_message(fd);
    for(vector<string>::size_type i = 0; i < res.size(); i++)
    {
        vector<string> key_value = split(res[i], "_");
        if(key_value.size() != 2)
        {
            continue;
        }

        if(key_value[0] == "parcel")
        {
            parcelID = key_value[1];
            parcelMap[parcelID]->isDelivered = true;
            parcelMap[parcelID]->end_time = time(NULL);
        }

        if(key_value[0] == "email")
        {
            userEmail = key_value[1];
        }
    }
    if(parcelID.size() > 0)
    {
        log() << "包裹 " << parcelID << " 由用户 " << userEmail 
            << " 送达目的地 " << endl;
        send(fd, "1", 1, 0);
    }
    send(fd, "0", 1, 0);
}

void add_parcel(Parcel *parcel)
{
    parcelMap[parcel->id] = parcel;
    if(stationSet.find(parcel->src) == stationSet.end() ||
            stationSet.find(parcel->dst) == stationSet.end())
    {
        log() << " 起始车站或是目的车站错误 " << endl;
        delete parcel;
        return;
    }
    string od = parcel->src + "_" + parcel->dst;
    if(parcelInLine.find(od) == parcelInLine.end())
    {
        parcelInLine[od] = set<Parcel *>();
    }
    parcelInLine[od].insert(parcel);
    log() << " 增加一个从 " << parcel->src << " 到 " << parcel->dst 
        << " 的包裹 " << parcel->id << endl; 
}

void read_station(string file)
{
    ifstream in;
    in.open(file.data(), ios::in);
    if(!in.is_open())
    {
        log() << " 无法打开文件 " << file << endl;
        return;
    }
    string line;
    while(getline(in, line))
    {
        vector<string> res = split(line, ":");
        stationSet.insert(res[0]);
    }
    in.close();
    
}
