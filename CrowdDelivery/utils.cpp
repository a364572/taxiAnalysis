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
#include<sstream>
#include"Point.h"
using namespace std;

map<string, User*> userMap;
map<string, Parcel*> parcelMap;
//map<string, set<Parcel*> > parcelInLine;
map<string, set<Parcel*> > parcelInStation;
map<string, Point*> stationSet;

string Parcel::generateID()
{
    string id = "";
    for(int i = 0; i < PARCEL_ID_LENGTH; i++)
    {
        int index = random() % 9; 
        id.push_back('0' + index + 1);
    }
    return id;
}
Parcel::Parcel()
{
    isDelivered = isCarried = false;
    src = dst = "";
    start_time = time(NULL);
    end_time = 0;
    id = generateID();
    while(parcelMap.find(id) != parcelMap.end())
    {
        id = generateID();
    }
    //times.push_back(start_time);
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

template<class T>
string toString(T t)
{
    string str;
    stringstream ss;
    ss << t;
    ss >> str;
    return str;
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
        log() << "投递包裹" << endl;
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
        send(fd, "exist_1", 7, 0);
        log() << " 用户注册失败 邮箱已经存在 " << user->email << endl;
        delete user;
    }
}

//得到某条路径的包裹数量
void handle_get_route_parcel(int fd, map<string, string>& keyValue)
{
    int cnt = 0;
    string parcel = "1";

    if(keyValue.find("src") == keyValue.end() || keyValue.find("dst") == keyValue.end())
    {
        send(fd, "cnt_0 parcel_0", 5, 0);
        return; 
    }
    string src = keyValue["src"];
    string dst = keyValue["dst"];
    get_parcel_count(src, dst, parcel, cnt);

    string response = "cnt_" + toString(cnt);
    response += " parcel_" + parcel;
    log() << "路径 " << src << " 到 " << dst << " 当前有 " << cnt << " 个包裹 " << parcel << endl;
    send(fd, response.data(), response.size(), 0);
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
        response = "exist_none";
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

    if(keyValue.find("email") == keyValue.end() ||
            keyValue.find("src") == keyValue.end() ||
            keyValue.find("dst") == keyValue.end())
    {
        log() << "请求信息不全" << endl;
        send(fd, "0", 1, 0);
        return;
    }
    int cnt = 0;
    string parcel;
    string src = keyValue["src"];
    string dst = keyValue["dst"];
    string email = keyValue["email"];
    get_parcel_count(src, dst, parcel, cnt);
    if(parcel.size() == PARCEL_ID_LENGTH)
    {
        time_t t = time(NULL);
        parcelMap[parcel]->times.push_back(t);
        parcelMap[parcel]->stations.push_back(src);
        parcelMap[parcel]->isCarried = true;
        log() << "包裹 " << parcel << " 即将送往" << dst << endl;
    }
    if(parcel.size() > 0)
    {
        string msg = "parcel_" + parcel;
        send(fd, msg.data(), msg.size(), 0);
        return;
    }

    if(userMap.find(email) == userMap.end())
    {
        log() << "没有该用户" << email << endl;
        send(fd, "0", 1, 0);
        return;
    }
    parcel = userMap[email]->current_parcel_id;
    if(parcel.size() < PARCEL_ID_LENGTH) 
    {
        map<string, set<Parcel*> >::iterator ite = parcelInStation.find(src);
        if(ite != parcelInStation.end())
        {
            set<Parcel *>::iterator parcelIte = ite->second.begin();
            while(parcelIte != ite->second.end())
            {
                if((*parcelIte)->dst == dst && !(*parcelIte)->isDelivered && !(*parcelIte)->isCarried)
                {
                    parcel = (*parcelIte)->id;
                    break;
                }
                parcelIte++;
            }
        }
    }

    if(parcel.size() == PARCEL_ID_LENGTH)
    {
        userMap[email]->src = parcelMap[parcel]->src;
        userMap[email]->dst = parcelMap[parcel]->dst;
        userMap[email]->current_parcel_id = parcel;
        parcelMap[parcel]->isCarried = true;
        parcel = "parcel_" + parcel + " src_" + parcelMap[parcel]->src + " dst_" + parcelMap[parcel]->dst;
        log() << "包裹 " << parcel << " 被指派给用户" << email << endl;
    }
    else
    {
        parcel = "parcel_1";
    }
    send(fd, parcel.data(), parcel.size(), 0);
}


//用户送达包裹
void handle_deliver_parcel(int fd, map<string, string>& keyValue)
{

    if(keyValue.find("parcel") == keyValue.end() ||
            keyValue.find("email") == keyValue.end() ||
            keyValue.find("time") == keyValue.end() ||
            keyValue.find("station") == keyValue.end())
    {
        log() << "请求信息不全" << endl;
        send(fd, "parcel_0", 8, 0);
        return;
    }

    string parcelID = keyValue["parcel"];
    string userEmail = keyValue["email"];
    string station = keyValue["station"];
    if(station.size() > 0)
    {
        time_t t = time(NULL);
        if(parcelMap.find(parcelID) != parcelMap.end() && 
                parcelMap[parcelID]->stations.size() > 0)
        {
            auto parcel = parcelMap[parcelID];
            //改变包裹所属车站
            string pre_station = parcel->stations.back();
            parcelInStation[pre_station].erase(parcelInStation[pre_station].find(parcel));
            if(parcelInStation.find(station) == parcelInStation.end())
            {
                parcelInStation[station] = set<Parcel*>();
            }
            parcelInStation[station].insert(parcel);
            parcel->times.push_back(t);
            parcel->stations.push_back(station);
            parcel->isCarried = false;
            if(parcel->dst == station)
            {
                parcel->isDelivered = true;
                log() << "包裹 " <<parcelID << " 成功投递至目的地" << endl;
            }
            else
            {
                log() << "包裹 " <<parcelID << " 投递至中转车站 " << station << endl;
            }
        }
        else
        {
            log() << "包裹 " <<parcelID << " 投递失败" << endl;
            parcelID = "1";
        }
        string msg = "parcel_" + parcelID;
        send(fd, msg.data(), msg.size(), 0);
        return;
    }

    string time = keyValue["time"];
    struct tm tm_info;
    if(strptime(time.data(), "%Y-%m-%d|%H:%M:%S", &tm_info) == NULL)
    {
        log() << "时间格式错误" << time << endl;
        send(fd, "parcel_0", 8, 0);
        return;
    }

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
    parcelMap[parcelID]->end_time = mktime(&tm_info);

    log() << "包裹 " << parcelID << " 由用户 " << userEmail 
        << " 送达目的地 时间 " << time << endl;
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
    int cnt = 0;
    string parcel;
    string src = keyValue["src"];
    string dst = keyValue["dst"];
    get_parcel_count(src, dst, parcel, cnt);

    userMap[email]->src = "src";
    userMap[email]->dst = "dst";
    char tmp[32];
    memset(tmp, 0, sizeof(tmp));
    sprintf(tmp, "cnt_%d src_%s dst_%s", cnt, src.data(), dst.data());
    send(fd, tmp, strlen(tmp), 0);
}

void add_parcel(Parcel *parcel, int flag)
{
    parcelMap[parcel->id] = parcel;
    if(stationSet.find(parcel->src) == stationSet.end() ||
            stationSet.find(parcel->dst) == stationSet.end())
    {
        log() << " 起始车站或是目的车站错误 " << endl;
        delete parcel;
        return;
    }
    if(parcelInStation.find(parcel->src) == parcelInStation.end())
    {
        parcelInStation[parcel->src] = set<Parcel*>();
    }
    if(flag == 0)
    {
        parcel->times.push_back(time(NULL));
        parcel->stations.push_back(parcel->src);
    }
    parcelInStation[parcel->src].insert(parcel);
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
        Point *p = new Point(res[1]);
        stationSet[res[0]] = p;
    }
    in.close();
}

void read_parcels(string file)
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
        vector<string> vec = split(line, " ");
        if(vec.size() >= 7)
        {
            Parcel *parcel  = new Parcel();
            parcel->id = vec[0];
            parcel->src = vec[1];
            parcel->dst = vec[2];
            if(vec[3] == "1") 
            {
                parcel->isCarried = true;
            }
            if(vec[4] == "1") 
            {
                parcel->isDelivered = true;
            }
            auto times = split(vec[5], "_");
            for(auto t : times)
            {
                long time;
                stringstream ss;
                ss << t;
                ss >> time;
                parcel->times.push_back((time_t)time);
            }
            auto stations = split(vec[6], "_");
            for(auto s : stations)
            {
                parcel->stations.push_back(s);
            }

            parcelMap[vec[0]] = parcel;
            if(!parcel->isDelivered)
            {
                if(parcelInStation.find(stations.back()) == parcelInStation.end())
                {
                    parcelInStation[stations.back()] = set<Parcel *>();
                }
                parcelInStation[stations.back()].insert(parcel);
            }
        }
    }
    in.close();
}
void read_users(string file)
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
        vector<string> vec = split(line, " ");
        if(vec.size() >= 7)
        {
            if(userMap.find(vec[0]) != userMap.end())
            {
                continue;
            }
            User *user = new User();
            user->name = vec[0];
            user->email = vec[1];
            user->telephone = vec[2];
            user->gender = vec[3];
            user->src = vec[4];
            user->dst = vec[5];
            user->current_parcel_id = vec[6];
            userMap[vec[1]] = user;
        }
    }
    in.close();
}

void write_users(string file)
{
    ofstream out;
    out.open(file.data(), ios::out);
    if(!out.is_open())
    {
        log() << " 无法打开文件 " << file << endl;
        return;
    }
    map<string, User*>::iterator ite = userMap.begin();
    while(ite != userMap.end())
    {
        User* user = ite->second;
        string line = user->name + " " + user->email + " " + user->telephone +
            " " + user->gender + " " + user->src + " " + user->dst + " " +
            user->current_parcel_id + "\n";
        out.write(line.data(), line.size());
        ite++;
    }
    out.close();
}
void write_parcels(string file)
{
    string file_append = file + "_delivered";
    ofstream out, out_append;
    out.open(file.data(), ios::out);
    out_append.open(file_append.data(), ios::out | ios::app);
    if(!out.is_open())
    {
        log() << " 无法打开文件 " << file << endl;
        return;
    }
    if(!out_append.is_open())
    {
        log() << " 无法打开文件 " << file_append << endl;
        return;
    }
    map<string, Parcel*>::iterator ite = parcelMap.begin();
    while(ite != parcelMap.end())
    {
        Parcel* parcel = ite->second;
        string line = parcel->id + " " + parcel->src + " " + parcel->dst;
        if(parcel->isCarried)
        {
            line += " 1";
        }
        else
        {
            line += " 0";
        }
        if(parcel->isDelivered)
        {
            line += " 1 ";
        }
        else
        {
            line += " 0 ";
        }
        for(vector<time_t>::size_type i = 0; i < parcel->times.size(); i++)
        {
            if(i == parcel->times.size() - 1)
            {
                line += toString(parcel->times.at(i)) + " ";
            }
            else
            {
                line += toString(parcel->times.at(i)) + "_";
            }
        }
        for(vector<string>::size_type i = 0; i < parcel->stations.size(); i++)
        {
            if(i == parcel->stations.size() - 1)
            {
                line += toString(parcel->stations.at(i)) + "\n";
            }
            else
            {
                line += toString(parcel->stations.at(i)) + "_";
            }
        }
        if(parcel->isDelivered)
        {
            out_append.write(line.data(), line.size());
        }
        else
        {
            out.write(line.data(), line.size());
        }
        ite++;
    }
    out_append.close();
}

//得到某个线路的包裹个数和第一个合适的包裹
void get_parcel_count(string src, string dst, string& id, int& cnt)
{
    cnt = 0;
    id = "1";
    if(parcelInStation.find(src) == parcelInStation.end())
    {
        parcelInStation[src] = set<Parcel *>();
    }
    auto ite = parcelInStation[src].begin();
    while(ite != parcelInStation[src].end())
    {
        if(!(*ite)->isDelivered && !(*ite)->isCarried && (*ite)->dst == dst)
        {
            cnt++;
            if(cnt == 1)
            {
                id = (*ite)->id;
            }
        }
        ite++;
    }
}
