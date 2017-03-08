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
#include"Point.h"
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
    start_time = get_time();
    end_time = 0;
    id = generateID();
    srv_type = Medium;
    while(parcelMap.find(id) != parcelMap.end())
    {
        id = generateID();
    }
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
        send_all_message(fd, msg);
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
        send_all_message(fd, response);

        log() << " 用户注册成功" << "\r\n\t姓名:" << user->name <<
            "\r\n\t性别:" << user->gender << "\r\n\t邮箱:" << user->email <<
            "\r\n\t电话:" << user->telephone << "\r\n\t起点:" <<
            user->src << "\r\n\t终点:" << user->dst << endl;
    }
    else
    {
        string response = "exist_1";
        send_all_message(fd, response);
        log() << " 用户注册失败 邮箱已经存在 " << user->email << endl;
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
        send_all_message(fd, msg);
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
    send_all_message(fd, response);
}

//获取用户正在派送的包裹
void handle_get_deliver_parcel(int fd, map<string, string>& keyValue)
{
    if(keyValue.find("email") == keyValue.end())
    {
        log() << "没有用户邮箱" << endl;
        string msg = "parcel_0";
        send_all_message(fd, msg);
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
    send_all_message(fd, parcelID);
}

//获取用户的信息 工作人员和乘客
void handle_get_user_information(int fd, map<string, string>& keyValue)
{
    string response = "0";

    if(keyValue.find("email") == keyValue.end())
    {
        send_all_message(fd, response);
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
    send_all_message(fd, response);
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
        send_all_message(fd, msg);
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
    get_parcel_count(src, dst, parcel, cnt);
    if(parcel.size() == PARCEL_ID_LENGTH)
    {
        time_t t = get_time();
        parcelMap[parcel]->times.push_back(t);
        parcelMap[parcel]->stations.push_back(src);
        parcelMap[parcel]->isCarried = true;
        log() << "包裹 " << parcel << " 即将送往 " << dst << " 实际目的地 " << parcelMap[parcel]->dst << endl;
    }
    else
    {
        log() << "当前没有合适的包裹" << endl;
    }
    //工作人员的话可以直接返回
    if(parcel.size() > 0)
    {
        string msg = "parcel_" + parcel;
        send_all_message(fd, msg);
        return;
    }

    if(userMap.find(email) == userMap.end())
    {
        log() << "没有该用户" << email << endl;
        string msg = "0";
        send_all_message(fd, msg);
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
    send_all_message(fd, parcel);
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
        send_all_message(fd, msg);
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
            log() << "包裹 " << parcelID << " 投递失败" << endl;
            parcelID = "0";
        }
        string msg = "parcel_" + parcelID;
        send_all_message(fd, msg);
        return;
    }

    string time = keyValue["time"];
    struct tm tm_info;
    if(strptime(time.data(), "%Y-%m-%d|%H:%M:%S", &tm_info) == NULL)
    {
        log() << "时间格式错误" << time << endl;
        string msg = "parcel_0";
        send_all_message(fd, msg);
        return;
    }

    if(userMap.find(userEmail) == userMap.end())
    {
        log() << "没有该用户" << userEmail << endl;
        string msg = "0";
        send_all_message(fd, msg);
        return;
    }
    if(parcelMap.find(parcelID) == parcelMap.end())
    {
        log() << "没有该包裹" << parcelID <<  endl;
        string msg = "0";
        send_all_message(fd, msg);
        return;
    }

    userMap[userEmail]->current_parcel_id = "0";
    parcelMap[parcelID]->isDelivered = true;
    parcelMap[parcelID]->isCarried= false;
    parcelMap[parcelID]->end_time = mktime(&tm_info);

    log() << "包裹 " << parcelID << " 由用户 " << userEmail 
        << " 送达目的地 时间 " << time << endl;
    string response = "parcel_" + parcelID;
    send_all_message(fd, response);
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
        send_all_message(fd, msg);
        return;
    }
    string email = keyValue["email"];
    if(userMap.find(email) == userMap.end())
    {
        log() << "没有该用户" << email << endl;
        string msg = "0";
        send_all_message(fd, msg);
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
    send_all_message(fd, msg);
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

//判断包裹是否要交给乘客，0:跳过，1:用我们的方法，2:Cost-oriented方法，3:time-oriented方法
static int is_worth_delivering(int sid_i, int did_i, int place_hour_i, int place_min_i,
        int place_sec_i, int srv_type_i, int cour_did_i, int cour_hour_i, int cour_min_i,
        int cour_sec_i)
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
    if(!sid || !did || !place_hour|| !place_min || !place_sec || !srv_type ||
            !cour_did || ! cour_hour || !cour_min || !cour_sec)
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
    //log() << sid_i << " " << did_i << " " << place_hour_i << " " << place_min_i << " " << place_sec_i << " " << srv_type_i << " " <<
    //    cour_did_i << " " << cour_hour_i << " " << cour_min_i << " " << cour_sec_i << endl;
        
    engEvalString(ep, "result=subway(sid, did, place_hour, place_min, place_sec, srv_type, cour_did, cour_hour, cour_min, cour_sec);");

    int res = 0;
    mxArray *result = engGetVariable(ep, "result");
    if(result)
    {
        res = mxGetPr(result)[0];
        mxDestroyArray(result);
    }
    return res;
}

//得到某个线路的包裹个数和第一个合适的包裹 参数是乘客的起点和目的地
void get_parcel_count(string src, string dst, string& id, int& cnt)
{
    cnt = 0;
    id = "1";
    if(parcelInStation.find(src) == parcelInStation.end())
    {
        parcelInStation[src] = set<Parcel *>();
        return;
    }
    //获取当前时间
    time_t now = get_time();
    struct tm current;
    struct tm *tm = localtime(&now);
    memcpy(&current, tm, sizeof(current));
    int cour_did = stationStrToInt[dst];
    int cour_hour = current.tm_hour;
    int cour_min = current.tm_min;
    int cour_sec = current.tm_sec;

    auto ite = parcelInStation[src].begin();
    while(ite != parcelInStation[src].end())
    {
        auto parcel = *ite;
        time_t arrive = parcel->times.back();
        //包裹没到达目的地，没被派送，到达时间早于当前时间
        if(!parcel->isDelivered && !parcel->isCarried && now >= arrive)
        {
            //包裹的起始时间是最开始的时间还是到达某个站点的时间?
            tm = localtime(&arrive);
            int sid = stationStrToInt[src];
            int did = stationStrToInt[parcel->dst];
            int place_hour = tm->tm_hour;
            int place_min = tm->tm_min;
            int place_sec = tm->tm_sec;

            int res = is_worth_delivering(sid, did, place_hour, place_min, place_sec, 
                    parcel->srv_type, cour_did, cour_hour, cour_min, cour_sec); 
            if(res > 0)
            {
                //不同日期选择不同的路由方法
                if(current.tm_mday % 3 == 0 && res % 2 == 1)
                {
                    log() << "data-oriented 的方法成功" << endl;
                    cnt++;
                }
                if(current.tm_mday % 3 == 1 && (res / 10) % 2 == 1)
                {
                    log() << "cost-oriented 的方法成功" << endl;
                    cnt++;
                }
                if(current.tm_mday % 3 == 2 && (res / 100) % 2 == 1)
                {
                    log() << "time-oriented 的方法成功" << endl;
                    cnt++;
                }
                if(cnt == 1)
                {
                    id = parcel->id;
                    break;
                }
            }
        }
        ite++;
    }
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
    engEvalString(ep, "cd ~/workplace/matlab/mfile;");
    engEvalString(ep, "addpath('~/workplace/matlab/mfile');");
    return 0;
}
void exit_matlab()
{
    engClose(ep);
}
