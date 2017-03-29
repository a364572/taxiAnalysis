/**
 * 主要的功能函数 包裹服务器交互 基本工具
 * 服务器函数中只有带'工作人员'的函数在实验中使用
 **/
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
#include<algorithm>
#include"commom.h"
#include"engine.h"
using namespace std;

map<string, User*> userMap;
map<string, Parcel*> parcelMap;
map<string, set<Parcel*> > parcelInStation;

Engine *ep = NULL;   

template<class T>
string toString(T t)
{
    string str;
    stringstream ss;
    ss << t;
    ss >> str;
    return str;
}
string Parcel::generateID()
{
    //static int start_id = 100000;
    string id = "";
    for(int i = 0; i < PARCEL_ID_LENGTH; i++)
    {
        int index = random() % 9; 
        id.push_back('0' + index + 1);
    }
    //id = to_string(start_id++);
    return id;
}
Parcel::Parcel()
{
    isDelivered = isCarried = false;
    src = dst = "";
    start_time = get_time();
    end_time = 0;
    id = generateID();
    srv_type = Medium;
    route_type = Cost;
    while(parcelMap.find(id) != parcelMap.end())
    {
        id = generateID();
    }
}

void handle_message_type(int fd, int message_type, map<string, string>& keyValue)
{
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
void handle_message_in(int fd, string msg)
{
    if(msg.size() <= 1)
    {
        log() << "读取消息出错" << endl;
        return;
    }
    int message_type = msg[0];
    msg = msg.substr(1);
    map<string, string> keyValue = decodeRequese(msg);
    handle_message_type(fd, message_type, keyValue);
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
    handle_message_type(fd, message_type, keyValue);
}

//用户注册 工作人员和乘客
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
        string msg = "0";
        send_all_message(fd, msg, 0);
        return;
    }
    User* user = new User();
    user->name = keyValue["name"];
    user->email = keyValue["email"];
    user->telephone = keyValue["telephone"];
    user->gender = keyValue["gender"];
    int srcIndex = atoi(keyValue["src"].data());
    int dstIndex = atoi(keyValue["dst"].data());
    user->src = stationIntToStr[srcIndex];
    user->dst = stationIntToStr[dstIndex];
    if(userMap.find(user->email) == userMap.end())
    {
        userMap[user->email] = user;
        string response = "email_" + user->email + " name_" + user->name;
        send_all_message(fd, response, 0);

        log() << " 用户注册成功" << "\r\n\t姓名:" << user->name <<
            "\r\n\t性别:" << user->gender << "\r\n\t邮箱:" << user->email <<
            "\r\n\t电话:" << user->telephone << "\r\n\t起点:" <<
            user->src << "\r\n\t终点:" << user->dst << endl;
    }
    else
    {
        log() << " 用户注册失败 邮箱已经存在 " << user->email << endl;
        string response = "exist_1";
        send_all_message(fd, response, 0);
        delete user;
    }
}

//得到某条路径的包裹数量 工作人员和乘客
void handle_get_route_parcel(int fd, map<string, string>& keyValue)
{
    int cnt = 0;
    string parcel = "1";

    if(keyValue.find("src") == keyValue.end() || keyValue.find("dst") == keyValue.end())
    {
        string msg = "cnt_0 parcel_1";
        send_all_message(fd, msg, 0);
        return; 
    }
    int srcIndex = atoi(keyValue["src"].data());
    int dstIndex = atoi(keyValue["dst"].data());
    string src = stationIntToStr[srcIndex];
    string dst = stationIntToStr[dstIndex];
    get_parcel_count(src, dst, parcel, cnt);

    string response = "cnt_" + toString(cnt);
    response += " parcel_" + parcel;
    log() << "路径 " << src << " 到 " << dst << " 当前有 " << cnt << " 个包裹 " << parcel << endl;
    send_all_message(fd, response, 0);
}

//获取用户正在派送的包裹
void handle_get_deliver_parcel(int fd, map<string, string>& keyValue)
{
    if(keyValue.find("email") == keyValue.end())
    {
        log() << "没有用户邮箱" << endl;
        string msg = "parcel_0";
        send_all_message(fd, msg, 0);
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
    send_all_message(fd, parcelID, 0);
}

//获取用户的信息 工作人员和乘客
void handle_get_user_information(int fd, map<string, string>& keyValue)
{
    string response = "0";

    if(keyValue.find("email") == keyValue.end())
    {
        send_all_message(fd, response, 0);
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
    send_all_message(fd, response, 0);
}

//用户选择接收包裹 工作人员和乘客
void handle_accept_parcel(int fd, map<string, string>& keyValue)
{
    if(keyValue.find("email") == keyValue.end() ||
            keyValue.find("src") == keyValue.end() ||
            keyValue.find("dst") == keyValue.end())
    {
        log() << "请求信息不全" << endl;
        string msg = "0";
        send_all_message(fd, msg, 0);
        return;
    }
    //首先查询是否有符合当前线路的包裹
    int cnt = 0;
    string parcel;
    int srcIndex = atoi(keyValue["src"].data());
    int dstIndex = atoi(keyValue["dst"].data());
    string src = stationIntToStr[srcIndex];
    string dst = stationIntToStr[dstIndex];
    string email = keyValue["email"];
    //time_t current_t = stol(keyValue["time"]);
    time_t current_t = get_time();
    get_parcel_count(src, dst, parcel, cnt, current_t);
    auto parcels = split(parcel, "-");
    //for(auto p: parcels)
    //{
    //    if(p.size() == PARCEL_ID_LENGTH && !parcelMap[p]->isCarried
    //        && !parcelMap[p]->isDelivered)
    //    {
    //        time_t t = get_time();
    //        if(keyValue.find("time") != keyValue.end())
    //        {
    //    	t = (current_t);
    //        }
    //        parcelMap[p]->times.push_back(t);
    //        parcelMap[p]->stations.push_back(src);
    //        parcelMap[p]->isCarried = true;
    //        log() << "包裹 " << p << " 即将从 " << src << " 送往 " << dst << " 实际目的地 " << parcelMap[p]->dst << endl;
    //    }
    //    else
    //    {
    //        log() << "当前没有合适的包裹" << endl;
    //    }
    //}
    //工作人员的话可以直接返回
    //if(parcel.size() > 0)
    //{
    //    string msg = "parcel_" + parcel;
    //    send_all_message(fd, msg, 0);
    //    return;
    //}

    if(userMap.find(email) == userMap.end())
    {
        log() << "没有该用户" << email << endl;
        string msg = "0";
        send_all_message(fd, msg, 0);
        return;
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
    send_all_message(fd, parcel, 0);
}


//用户送达包裹 工作人员和乘客
void handle_deliver_parcel(int fd, map<string, string>& keyValue)
{
    if(keyValue.find("parcel") == keyValue.end() ||
            keyValue.find("email") == keyValue.end() ||
            keyValue.find("time") == keyValue.end() ||
            keyValue.find("station") == keyValue.end())
    {
        log() << "请求信息不全" << endl;
        string msg = "parcel_0";
        send_all_message(fd, msg, 0);
        return;
    }

    int stationIndex = atoi(keyValue["station"].data());
    string station = stationIntToStr[stationIndex];
    string parcelID = keyValue["parcel"];
    string userEmail = keyValue["email"];
    //工作人员才带有station字段
    if(station.size() > 0)
    {
        time_t t = get_time();
	if(keyValue.find("time") != keyValue.end())
	{
		t = stol(keyValue["time"]);
	}
        if(parcelMap.find(parcelID) != parcelMap.end() && 
                parcelMap[parcelID]->stations.size() > 0)
        {
            auto parcel = parcelMap[parcelID];
            //改变包裹所属车站
            if(find(parcel->stations.begin(), parcel->stations.end(), station) != parcel->stations.end() ||
                    parcel->isDelivered)
            {
                log() << "包裹 " << parcelID << " 投递失败 重复投递" << endl;
                parcelID = "0";
            }
            else
            {
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
        }
        else
        {
            log() << "包裹 " << parcelID << " 投递失败" << endl;
            parcelID = "0";
        }
        string msg = "parcel_" + parcelID;
        send_all_message(fd, msg, 0);
        return;
    }

    string time = keyValue["time"];
    struct tm tm_info;
    if(strptime(time.data(), "%Y-%m-%d|%H:%M:%S", &tm_info) == NULL)
    {
        log() << "时间格式错误" << time << endl;
        string msg = "parcel_0";
        send_all_message(fd, msg, 0);
        return;
    }

    if(userMap.find(userEmail) == userMap.end())
    {
        log() << "没有该用户" << userEmail << endl;
        string msg = "0";
        send_all_message(fd, msg, 0);
        return;
    }
    if(parcelMap.find(parcelID) == parcelMap.end())
    {
        log() << "没有该包裹" << parcelID <<  endl;
        string msg = "0";
        send_all_message(fd, msg, 0);
        return;
    }

    userMap[userEmail]->current_parcel_id = "0";
    parcelMap[parcelID]->isDelivered = true;
    parcelMap[parcelID]->isCarried= false;
    parcelMap[parcelID]->end_time = mktime(&tm_info);

    log() << "包裹 " << parcelID << " 由用户 " << userEmail 
        << " 送达目的地 时间 " << time << endl;
    string response = "parcel_" + parcelID;
    send_all_message(fd, response, 0);
}

//乘客改变日常出行路径
void handle_change_route(int fd, map<string, string>& keyValue)
{
    if(keyValue.find("email") == keyValue.end() ||
            keyValue.find("src") == keyValue.end() ||
            keyValue.find("dst") == keyValue.end())
    {
        log() << "请求信息不全" << endl;
        string msg = "0";
        send_all_message(fd, msg, 0);
        return;
    }
    string email = keyValue["email"];
    if(userMap.find(email) == userMap.end())
    {
        log() << "没有该用户" << email << endl;
        string msg = "0";
        send_all_message(fd, msg, 0);
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
    string msg = tmp;
    send_all_message(fd, msg, 0);
}

//flag表示是否已经包含时间 为1表示parcel已经构造好了时间
void add_parcel(Parcel *parcel, int flag)
{
    if(stationStrToInt.find(parcel->src) == stationStrToInt.end() ||
            stationStrToInt.find(parcel->dst) == stationStrToInt.end())
    {
        log() << " 起始车站或是目的车站错误 " << endl;
        delete parcel;
        return;
    }
    if(parcelMap.find(parcel->id) != parcelMap.end())
    {
	log() << "包裹" << parcel->id << "已经存在" << endl;
	delete parcel;
	return;
    }

    parcelMap[parcel->id] = parcel;
    if(parcelInStation.find(parcel->src) == parcelInStation.end())
    {
        parcelInStation[parcel->src] = set<Parcel*>();
    }
    if(flag == 0)
    {
        parcel->times.push_back(get_time());
        parcel->stations.push_back(parcel->src);
    }
    parcelInStation[parcel->src].insert(parcel);
    log() << " 增加一个从 " << parcel->src << " 到 " << parcel->dst 
        << " 的包裹 " << parcel->id << " 起始时间 " << parcel->times.back() << endl; 
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
            if(vec[3] == "3")
            {
                parcel->srv_type = Parcel::ParcelServerType::Fast;
            }
            else if(vec[3] == "2")
            {
                parcel->srv_type = Parcel::ParcelServerType::Medium;
            }
            else
            {
                parcel->srv_type = Parcel::ParcelServerType::Slow;
            }
            if(vec[4] == "2")
            {
                parcel->route_type = Parcel::ParcelRouteType::Best;
            }
            else if(vec[4] == "1")
            {
                parcel->route_type = Parcel::ParcelRouteType::Cost;
            }
            else
            {
                parcel->route_type = Parcel::ParcelRouteType::Data;
            }

            if(vec[5] == "1") 
            {
                parcel->isCarried = true;
            }
            if(vec[6] == "1") 
            {
                parcel->isDelivered = true;
            }
            auto times = split(vec[7], "_");
            struct tm tm;
            for(auto t : times)
            {
                strptime(t.data(), "%Y-%m-%d|%H:%M:%S", &tm);
                parcel->times.push_back(mktime(&tm));
            }
            auto stations = split(vec[8], "_");
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

//包裹输出格式 ID,起点，目的点，服务类型，路由方法类型，是否被携带，是否送达，各个时间点，各个车站
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
        string line = parcel->id + " " + parcel->src + " " + parcel->dst + " " + 
            to_string(parcel->srv_type) + " " + to_string(parcel->route_type);
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
        char tmp[64];
        for(vector<time_t>::size_type i = 0; i < parcel->times.size(); i++)
        {
	    memset(tmp, 0, sizeof(tmp));
	    struct tm* tm = localtime(&(parcel->times.at(i)));
	    strftime(tmp, sizeof(tmp), "%Y-%m-%d|%H:%M:%S", tm);
	    line += tmp;
            if(i == parcel->times.size() - 1)
            {
                line += " ";
            }
            else
            {
                line += "_";
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

//判断包裹是否要交给乘客，method : 0:用我们的方法，1:Cost-oriented方法，2:best-oriented方法
static int is_worth_delivering(int sid_i, int did_i, int place_hour_i, int place_min_i,
        int place_sec_i, int srv_type_i, int cour_did_i, int cour_hour_i, int cour_min_i,
        int cour_sec_i, int method_i)
{
    if(!ep)
    {
        log() << "matlab引擎未启动" << endl;
        return 0;
    }
    if((sid_i < 8 || sid_i == 11) && (cour_did_i > 12 || cour_did_i == 8))
    {
        return 0;
    }
    if((sid_i > 12 || sid_i == 8) && (cour_did_i < 8 || cour_did_i == 11))
    {
        return 0;
    }

    static mxArray *sid = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
    static mxArray *did = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
    static mxArray *place_hour = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
    static mxArray *place_min = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
    static mxArray *place_sec = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
    static mxArray *srv_type = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
    static mxArray *cour_did = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
    static mxArray *cour_hour = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
    static mxArray *cour_min = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
    static mxArray *cour_sec = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
    static mxArray *method = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
    if(!sid || !did || !place_hour|| !place_min || !place_sec || !srv_type ||
            !cour_did || ! cour_hour || !cour_min || !cour_sec || !method)
    {
        log() << "指针初始化失败" << endl;
        return 0;
    }
    memcpy(mxGetPr(sid), &sid_i, sizeof(int));
    memcpy(mxGetPr(did), &did_i, sizeof(int));
    memcpy(mxGetPr(place_hour), &place_hour_i, sizeof(int));
    memcpy(mxGetPr(place_min), &place_min_i, sizeof(int));
    memcpy(mxGetPr(place_sec), &place_sec_i, sizeof(int));
    memcpy(mxGetPr(srv_type), &srv_type_i, sizeof(int));
    memcpy(mxGetPr(cour_did), &cour_did_i, sizeof(int));
    memcpy(mxGetPr(cour_hour), &cour_hour_i, sizeof(int));
    memcpy(mxGetPr(cour_min), &cour_min_i, sizeof(int));
    memcpy(mxGetPr(cour_sec), &cour_sec_i, sizeof(int));
    memcpy(mxGetPr(method), &method_i, sizeof(int));
    engPutVariable(ep, "sid", sid);  
    engPutVariable(ep, "did", did);  
    engPutVariable(ep, "place_hour", place_hour);  
    engPutVariable(ep, "place_min", place_min);  
    engPutVariable(ep, "place_sec", place_sec);  
    engPutVariable(ep, "srv_type", srv_type);  
    engPutVariable(ep, "cour_did", cour_did);  
    engPutVariable(ep, "cour_hour", cour_hour);  
    engPutVariable(ep, "cour_min", cour_min);  
    engPutVariable(ep, "cour_sec", cour_sec);  
    engPutVariable(ep, "method", method);  
    //log() << sid_i << " " << did_i << " " << place_hour_i << " " << place_min_i << " " << place_sec_i << " " << srv_type_i << " " <<
    //    cour_did_i << " " << cour_hour_i << " " << cour_min_i << " " << cour_sec_i << endl;
        
    engEvalString(ep, "result=subway(sid, did, place_hour, place_min, place_sec, srv_type, cour_did, cour_hour, cour_min, cour_sec, method);");

    int res = 0;
    mxArray *result = engGetVariable(ep, "result");
    if(result)
    {
        res = mxGetPr(result)[0];
        mxDestroyArray(result);
    }
    return res;
}

static void swap(vector<Parcel *>&vec, int i, int j)
{
    auto parcel = vec[i];
    vec[i] = vec[j];
    vec[j] = parcel;
}
//得到某个线路的包裹个数所有合适的包裹 参数是乘客的起点和目的地
void get_parcel_count(string src, string dst, string& id, int&cnt)
{
	get_parcel_count(src, dst, id, cnt, get_time());
}
void get_parcel_count(string src, string dst, string& id, int& cnt, time_t now)
{
    cnt = 0;
    id = "1";
    if(parcelInStation.find(src) == parcelInStation.end())
    {
        parcelInStation[src] = set<Parcel *>();
    }
    //获取当前时间
    struct tm current;
    struct tm *tm = localtime(&now);
    memcpy(&current, tm, sizeof(current));
    int cour_did = stationStrToInt[dst];
    int cour_hour = current.tm_hour;
    int cour_min = current.tm_min;
    int cour_sec = current.tm_sec;

    //获取当前车站的包裹并排序
    vector<Parcel*> all_parcels;
    for(auto parcel : parcelInStation[src])
    {
        time_t arrive = parcel->times.back();
        //包裹没到达目的地，没被派送，到达时间早于当前时间
        if(parcel->isCarried || parcel->isDelivered || arrive > now
	    || find(parcel->stations.begin(), parcel->stations.end(), dst) != parcel->stations.end())
        {
            continue;
        }
        all_parcels.push_back(parcel);
    }
    for(auto parcel : all_parcels)
    {
	if(parcel->isCarried)
	{
	    log() << "error " << parcel->id<< endl;
	}
    }
    //将包裹按照不同的路由方法排序 方法一样的按照优先级排序 最后按照时间
    for(decltype(all_parcels.size()) i = 0; i < all_parcels.size(); i++)
    {
        for(decltype(i) j = 1; j < all_parcels.size() - i; j++)
        {
            auto first = all_parcels[j - 1];
            auto second = all_parcels[j];
            if(first->route_type < second->route_type)
            {
                swap(all_parcels, j - 1, j);
            }
            else if(first->route_type == second->route_type)
            {
                if(first->srv_type < second->srv_type)
                {
                    swap(all_parcels, j - 1, j);
                }
                else if(first->srv_type == second->srv_type)
                {
		    int f_sec = first->times.back();
		    int s_sec = second->times.back();
                    if(f_sec > s_sec)
                    {
                        swap(all_parcels, j - 1, j);
                    }
                }
            }
        }
    }
    int flag = 0;
    int p_method = -1;
    int p_srv = -1;
    int p_hour = -1;
    vector<Parcel*> cans;
    for(auto parcel : all_parcels)
    {
        time_t arrive = parcel->times.back();
        tm = localtime(&arrive);
        int sid = stationStrToInt[src];
        int did = stationStrToInt[parcel->dst];
        int place_hour = tm->tm_hour;
        int place_min = tm->tm_min;
        int place_sec = tm->tm_sec;

        //每天一种方法
        int method = parcel->route_type;
	int srv = parcel->srv_type;
        int carry = 1 << method;
        if(flag & carry)
        {
            continue;
        }
	if(p_method == method && p_hour == place_hour 
	    && p_srv == srv)
	{
	    continue;
	}

        int res = is_worth_delivering(sid, did, place_hour, place_min, place_sec, 
                srv, cour_did, cour_hour, cour_min, cour_sec, method); 
        if(res > 0)
        {
            flag |= carry;
            cans.push_back(parcel);
        }
	p_method = method;
	p_hour = place_hour;
	p_srv == srv;
    }
    cnt = cans.size();
    if(cnt == 0)
    {
        return;
    }
    id = "";
    string out;
    for(auto parcel : cans)
    {
        id += parcel->id;
        if(parcel != cans.back())
        {
            id += "-";
        }
        if(parcel->route_type == 0)
        {
            out += "data-oriented ";
        }
        if(parcel->route_type == 1)
        {
            out += "cost-oriented ";
        }
        if(parcel->route_type == 2)
        {
            out += "best-oriented ";
        }
    }
    log() << out << "的方法成功" << endl;
}
//初始化matlab引擎
int init_matlab()
{
    if(ep)
    {
        return 0;
    }
    if (!(ep = engOpen(NULL))) //if start successfully, the pointer ep will not be NULL.   
    {   
        log() << "Can't start MATLAB engine!"<<endl;   
        return -1;   
    }
        log() << "start MATLAB engine!"<<endl;   
    engEvalString(ep, "cd ~/workplace/matlab/mfile/github/Crowd-Delivery;");
    engEvalString(ep, "addpath('~/workplace/matlab/mfile/github/Crowd-Delivery');");
    return 0;
}
void exit_matlab()
{
    engClose(ep);
}

//随机产生包裹，总个数 开始小时 结束小时 左闭右开 line:1/2
void generate_parcels(int num, int beginHour, int endHour, int line)
{
    int total = 0;
    int interval = (endHour - beginHour) * 60 / num;
    int avg = num / 3;

    //总共两条线路 
    //  会展中心6->大剧院7->市民中心11
    //  老街8->少年宫13->布吉14->深圳北站15
    //获取当天日期
    time_t now = get_time();
    struct tm timeBegin;
    struct tm* tm = localtime(&now);
    memcpy(&timeBegin, tm, sizeof(timeBegin));
    timeBegin.tm_sec = timeBegin.tm_min = tm->tm_hour = 0;

    for(int cnt = 0; cnt < num; cnt++)
    {
	    vector<int> srcSet = {6, 7};
	    int srcIndex = 6;
	    srcIndex = random() % srcSet.size();
	    srcIndex = srcSet[srcIndex];
	    int dstIndex = 11;
	    if(line == 2)
	    {
		    srcSet = {8, 13, 14};
		    srcIndex = 8;
		    srcIndex = random() % srcSet.size();
		    srcIndex = srcSet[srcIndex];
		    srcIndex = 8;
		    dstIndex = 14;
	    }
        //先产生最慢的 然后中等的 最后最快的
        Parcel::ParcelServerType srv_type;
        if(cnt < avg)
        {
            srv_type = Parcel::ParcelServerType::Slow;
        }
        else if(cnt < avg * 2)
        {
            srv_type = Parcel::ParcelServerType::Medium;
        }
        else
        {
            srv_type = Parcel::ParcelServerType::Fast;
        }
        int nextMinute = cnt * interval;
        timeBegin.tm_hour = beginHour + nextMinute / 60;
        timeBegin.tm_min = nextMinute % 60;
        now = mktime(&timeBegin);
        //每个包裹产生3个 并用不同的方法传递
        for(int i = 1; i < 2; i++)
        {
            total++;
            Parcel *parcel = new Parcel();
            parcel->src = stationIntToStr[srcIndex];
            parcel->dst = stationIntToStr[dstIndex];
            parcel->srv_type = srv_type;

            if(i == 0)
            {
                parcel->route_type = Parcel::ParcelRouteType::Data;
            }
            else if(i == 1)
            {
                parcel->route_type = Parcel::ParcelRouteType::Cost;
            }
            else
            {
                parcel->route_type = Parcel::ParcelRouteType::Best;
            }
            //设置包裹的起始时间 起始站点
            parcel->start_time = now;
            parcel->times.push_back(now);
            parcel->stations.push_back(parcel->src);
            add_parcel(parcel, 1);
        }
    }
    log() << "共产生了 " << total << " 个包裹" << endl;
    log() << "Slow: " << avg * 3 << " Medium: " << avg * 3 << " Fast: " << total - 6 * avg << endl; 
}
