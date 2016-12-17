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
    isDelivered = isCarried = false;
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
map<string, string> decodeRequese(string line)
{
    map<string, string> result;
    vector<string> res = split(line, " ");
    vector<string>::iterator ite = res.begin();
    while(ite != res.end())
    {
        string msg = *ite;
        vector<string> vec = split(msg, "_");
        if(vec.size() == 2)
        {
            result[vec[0]] = vec[1];
        }
        ite++;
    }
    return result;
}


string recv_all_message(int fd)
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
        return "";
    }
    tmp[total_length] = '\0';
    return tmp;
}

void handle_message_in(int fd)
{
    int message_type = 0;
    if(fd < 0 || recv(fd, &message_type, 1, 0 ) != 1)
    {
        log() << "读取消息出错" << endl;
        return;
    }

    log() << "消息类型 " << message_type << endl;

    string msg = recv_all_message(fd);
    map<string, string> keyValue = decodeRequese(msg);
    switch(message_type)
    {
    case MESSAGE_REGISTER:
        log() << "新用户注册" << endl;
        handle_register(fd, keyValue);
        break;
    case MESSAGE_GET_ROUTE_PARCEL:
        log() << "获取包裹数量" << endl;
        handle_get_route_parcel(fd, keyValue);
        break;
    case MESSAGE_GET_DELIVER_PARCEL:
        log() << "获取当前包裹" << endl;
        handle_get_deliver_parcel(fd, keyValue);
        break;
    case MESSAGE_GET_USER_INFORMATION:
        log() << "获取用户信息" << endl;
        handle_get_user_information(fd, keyValue);
        break;
    case MESSAGE_ACCEPT_PARCEL:
        log() << "用户接受任务" << endl;
        handle_accept_parcel(fd, keyValue);
        break;
    case MESSAGE_DELIVER_PARCEL:
        log() << "成功投递包裹" << endl;
        handle_deliver_parcel(fd, keyValue);
        break;
    case MESSAGE_CHANGE_ROUTE:
        log() << "修改出行路径" << endl;
        handle_change_route(fd, keyValue);
        break;
    default:
        log() << "不合法的消息类型" << message_type << endl;
        break;
    }
}

//用户注册
void handle_register(int fd, map<string, string>& keyValue)
{
    if(keyValue.find("name") == keyValue.end() ||
            keyValue.find("email") == keyValue.end() ||
            keyValue.find("telephone") == keyValue.end() ||
            keyValue.find("gender") == keyValue.end() ||
            keyValue.find("src") == keyValue.end() ||
            keyValue.find("dst") == keyValue.end())
    {
        log() << "不完整的注册信息" << endl;
        send(fd, "0", 1, 0);
        return;
    }
    User* user = new User();
    user->name = keyValue["name"];
    user->email = keyValue["email"];
    user->telephone = keyValue["telephone"];
    user->gender = keyValue["gender"];
    user->src = keyValue["src"];
    user->dst = keyValue["dst"];
    if(userMap.find(user->email) == userMap.end())
    {
        userMap[user->email] = user;
        string response = "email_" + user->email + " name_" + user->name;
        send(fd, response.data(), response.size(), 0);

        log() << " 用户注册成功" << "\r\n\t姓名:" << user->name <<
            "\r\n\t性别:" << user->gender << "\r\n\t邮箱:" << user->email <<
            "\r\n\t电话:" << user->telephone << "\r\n\t起点:" <<
            user->src << "\r\n\t终点:" << user->dst << endl;
    }
    else
    {
        send(fd, "2", 1, 0);
        log() << " 用户注册失败 邮箱已经存在 " << user->email << endl;
        delete user;
    }
}

//得到某条路径的包裹数量
void handle_get_route_parcel(int fd, map<string, string>& keyValue)
{
    int cnt = 0;
    char tmp[16];

    if(keyValue.find("src") == keyValue.end() || keyValue.find("dst") == keyValue.end())
    {
        send(fd, "cnt_0", 5, 0);
        return; 
    }
    string route = keyValue["src"] + "|" + keyValue["dst"];
    map<string, set<Parcel*> >::iterator ite = parcelInLine.find(route);
    if(ite != parcelInLine.end())
    {
        set<Parcel *>::iterator parcelIte = ite->second.begin();
        while(parcelIte != ite->second.end())
        {
            if(!(*parcelIte)->isDelivered && !(*parcelIte)->isCarried)
            {
                cnt++;
            }
            parcelIte++;
        }
    }
    memset(tmp, 0, sizeof(tmp));
    sprintf(tmp, "cnt_%d", cnt);
    log() << "路径 " << route << " 当前有 " << cnt << " 个包裹" << endl;
    send(fd, tmp, strlen(tmp), 0);
}

//获取用户正在派送的包裹
void handle_get_deliver_parcel(int fd, map<string, string>& keyValue)
{
    if(keyValue.find("email") == keyValue.end())
    {
        log() << "没有用户邮箱" << endl;
        send(fd, "parcel_0", 8, 0);
        return;
    } 
    string email = keyValue["email"];
    string parcelID = "0";
    string od = " src_ dst_";
    map<string, User*>::iterator ite = userMap.find(email);
    if(ite != userMap.end())
    {
        parcelID = ite->second->current_parcel_id;
        od = " src_" + parcelMap[parcelID]->src + " dst_" + parcelMap[parcelID]->dst;
    }
    log() << " 用户 " << email << " 正在派送包裹" << parcelID << endl;
    parcelID = "parcel_" + parcelID + od;
    send(fd, parcelID.data(), parcelID.size(), 0);
}

//获取用户的信息
void handle_get_user_information(int fd, map<string, string>& keyValue)
{
    string response = "0";

    if(keyValue.find("email") == keyValue.end())
    {
        send(fd, "0", 1, 0);
        return;
    }
    string email = keyValue["email"];
    map<string, User*>::iterator ite = userMap.find(email);
    if(ite == userMap.end())
    {
        log() << "没有该用户" << email << endl;
        response = "0";
    }
    else
    {
        response = "name_" + ite->second->name;
        response += " email_" + ite->second->email;
        response += " telephone_" + ite->second->telephone;
        response += " parcel_" + ite->second->current_parcel_id;
        response += " src_" + ite->second->src;
        response += " dst_" + ite->second->dst;

        log() << "获取用户 " << email << " 信息成功 "
            << response <<  endl;
    }
    send(fd, response.data(), response.size(), 0);
}

//用户选择接收包裹
void handle_accept_parcel(int fd, map<string, string>& keyValue)
{
    string parcelID = "0";

    if(keyValue.find("email") == keyValue.end() ||
            keyValue.find("src") == keyValue.end() ||
            keyValue.find("dst") == keyValue.end())
    {
        log() << "请求信息不全" << endl;
        send(fd, "0", 1, 0);
        return;
    }

    string email = keyValue["email"];
    if(userMap.find(email) == userMap.end())
    {
        log() << "没有该用户" << email << endl;
        send(fd, "0", 1, 0);
        return;
    }
    parcelID = userMap[email]->current_parcel_id;
    if(parcelID == "0") 
    {
        string od = keyValue["src"] + "|" + keyValue["dst"];
        map<string, set<Parcel*> >::iterator ite = parcelInLine.find(od);
        if(ite != parcelInLine.end())
        {
            set<Parcel *>::iterator parcelIte = ite->second.begin();
            while(parcelIte != ite->second.end())
            {
                if(!(*parcelIte)->isDelivered && !(*parcelIte)->isCarried)
                {
                    parcelID = (*parcelIte)->id;
                    break;
                }
                parcelIte++;
            }
        }
    }

    if(parcelID.size() > 1)
    {
        userMap[email]->src = parcelMap[parcelID]->src;
        userMap[email]->dst = parcelMap[parcelID]->dst;
        userMap[email]->current_parcel_id = parcelID;
        parcelMap[parcelID]->isCarried = true;
        parcelID = "parcel_" + parcelID + " src_" + parcelMap[parcelID]->src + " dst_" + parcelMap[parcelID]->dst;
        log() << "包裹 " << parcelID << " 被指派给用户" << email << endl;
    }
    else
    {
        parcelID = "parcel_0";
    }
    send(fd, parcelID.data(), parcelID.size(), 0);
}


//用户送达包裹
void handle_deliver_parcel(int fd, map<string, string>& keyValue)
{
    string parcelID;
    string userEmail;

    if(keyValue.find("parcel") == keyValue.end() ||
            keyValue.find("email") == keyValue.end())
    {
        log() << "请求信息不全" << endl;
        send(fd, "0", 1, 0);
        return;
    }

    parcelID = keyValue["parcel"];
    userEmail = keyValue["email"];

    if(userMap.find(userEmail) == userMap.end())
    {
        log() << "没有该用户" << userEmail << endl;
        send(fd, "0", 1, 0);
        return;
    }
    if(parcelMap.find(parcelID) == parcelMap.end())
    {
        log() << "没有该包裹" << parcelID <<  endl;
        send(fd, "0", 1, 0);
        return;
    }

    userMap[userEmail]->current_parcel_id = "0";
    parcelMap[parcelID]->isDelivered = true;
    parcelMap[parcelID]->isCarried= false;
    parcelMap[parcelID]->end_time = time(NULL);

    log() << "包裹 " << parcelID << " 由用户 " << userEmail 
        << " 送达目的地 " << endl;
    string response = "parcel_" + parcelID;
    send(fd, response.data(), response.size(), 0);
}

void handle_change_route(int fd, map<string, string>& keyValue)
{
    if(keyValue.find("email") == keyValue.end() ||
            keyValue.find("src") == keyValue.end() ||
            keyValue.find("dst") == keyValue.end())
    {
        log() << "请求信息不全" << endl;
        send(fd, "0", 1, 0);
        return;
    }
    string email = keyValue["email"];
    if(userMap.find(email) == userMap.end())
    {
        log() << "没有该用户" << email << endl;
        send(fd, "0", 1, 0);
        return;
    }
    userMap[email]->src = keyValue["src"];
    userMap[email]->dst = keyValue["dst"];
    string route = keyValue["src"] + "|" + keyValue["dst"];
    map<string, set<Parcel*> >::iterator ite = parcelInLine.find(route);
    int cnt = 0;
    if(ite != parcelInLine.end())
    {
        set<Parcel *>::iterator parcelIte = ite->second.begin();
        while(parcelIte != ite->second.end())
        {
            if(!(*parcelIte)->isDelivered && !(*parcelIte)->isCarried)
            {
                cnt++;
            }
            parcelIte++;
        }
    }
    char tmp[32];
    memset(tmp, 0, sizeof(tmp));
    sprintf(tmp, "cnt_%d src_%s dst_%s", cnt, keyValue["src"].data(), keyValue["dst"].data());
    send(fd, tmp, strlen(tmp), 0);
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
    string od = parcel->src + "|" + parcel->dst;
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
