/**
 * 本程序用于模拟客户端 向服务器发起请求 每个客户端程序开始运行的时间是随机的
 **/
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/time.h>
#include<assert.h>
#include<string.h>

#include<string>
#include<vector>
#include<map>
#include<algorithm>
#include<fstream>
#include<iostream>
#include "utils.h"
#include "commom.h"

using namespace std;
#define COURIER_NUMBER 20       //每个车站的乘客数

map<string, int> stationStrToInt;
map<int, string> stationIntToStr;
ostream& newlog(time_t now)
{
    struct tm *tm = localtime(&now);
    char tmp[64];
    memset(tmp, 0, sizeof(tmp));
    sprintf(tmp, "%4d-%02d-%02d %02d:%02d:%02d : ", tm->tm_year + 1900, 
            tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    return cout << tmp;
}
struct Courier 
{
    time_t in_time;
    time_t out_time;
    string src;
    string dst;
    string parcel;
};

void read_record(map<int, vector<Courier>>& courier_map)
{
    ifstream in;
    in.open("simple", ios::in);
    if(!in.is_open())
    {
        newlog(get_time()) << "文件打开失败" << endl;
        return;
    }
    //获取今天的日期
    int cnt = 0;
    struct tm today;
    time_t now = get_time();
    struct tm* tm = localtime(&now);
    memcpy(&today, tm, sizeof(today));
    string line;
    while(getline(in, line))
    {
        auto vec = split(line, ",");
        if(vec.size() == 4)
        {
    //  会展中心6->大剧院7->市民中心11
    //  老街8->少年宫13->布吉14->深圳北站15
            int srcIndex = stationStrToInt[vec[1]];
            int dstIndex = stationStrToInt[vec[3]];
            if(srcIndex != 6 && srcIndex != 7 && srcIndex != 8 &&
                    srcIndex != 13 && srcIndex != 14)
            {
                continue;
            }
            if(dstIndex != 6 && dstIndex != 7 && dstIndex != 11 && dstIndex != 8 &&
                    dstIndex != 13 && dstIndex != 14 && dstIndex != 15)
            {
                continue;
            }
	    if(
	        (srcIndex == 8 && dstIndex == 13) ||
	        (srcIndex == 8 && dstIndex == 14) ||
	        (srcIndex == 8 && dstIndex == 15) ||
	        (srcIndex == 13 && dstIndex == 8) ||
	        (srcIndex == 13 && dstIndex == 14) ||
	        (srcIndex == 13 && dstIndex == 15) ||
	        (srcIndex == 14 && dstIndex == 8) ||
	        (srcIndex == 14 && dstIndex == 13) ||
	        (srcIndex == 14 && dstIndex == 15)) 
	    //if(srcIndex > 0)
	    {
            struct tm begin;
            struct tm end;
            time_t t1, t2;
            strptime(vec[0].data(), "%Y-%m-%d %H:%M:%S", &begin);
            strptime(vec[2].data(), "%Y-%m-%d %H:%M:%S", &end);
            if(begin.tm_hour < EMUL_START_HOUR || begin.tm_hour > EMUL_END_HOUR)
            {
                continue;
            }
            //以十分之一的概率采样
	    int sample_rate = 3;
	    //int sample_rate = 3;
	    if(srcIndex == 8 && dstIndex == 15)
	    {
		sample_rate = 6;
	    }
	    if(srcIndex == 15 && dstIndex == 8)
	    {
		sample_rate = 10;
	    }
            int left_prob = random() % sample_rate;
            if(left_prob == 0)
            {
                if(courier_map.find(srcIndex) == courier_map.end())
                {
                    courier_map[srcIndex] = vector<Courier>();
                }
                begin.tm_year = today.tm_year;
                begin.tm_mon = today.tm_mon;
                begin.tm_mday = today.tm_mday;
                end.tm_year = today.tm_year;
                end.tm_mon = today.tm_mon;
                end.tm_mday = today.tm_mday;
                t1 = mktime(&begin);
                t2 = mktime(&end);
                Courier courier;
                courier.src = stationIntToStr[srcIndex];
                courier.dst = stationIntToStr[dstIndex];
                courier.in_time = t1;
                courier.out_time = t2;
                courier.parcel = "0";
                courier_map[srcIndex].push_back(courier);
                cnt++;
            }
	    }
        }
    }
    newlog(get_time()) << "总共读取了 " << cnt << " 条记录" << endl;
    auto ite = courier_map.begin();
    while(ite != courier_map.end())
    {
	int index = ite->first;
	for(int i = 0; i < courier_map[index].size(); i++)
	{
	    for(int j = 1; j < courier_map[index].size() - i; j++)
	    {
		auto first = courier_map[index][j - 1];
		auto second = courier_map[index][j];
		if(first.in_time > second.in_time)
		{
			auto cour = courier_map[index][j - 1];
			courier_map[index][j - 1] = courier_map[index][j];
			courier_map[index][j] = cour;
		}
	    }
	}
	ite++;
    }
}
int get_socket_fd()
{
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(fd < 0)
    {
        return fd;
    }
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(SERVER_PORT);
    inet_aton(SERVER_IP, &sockaddr.sin_addr);
//    inet_aton("192.168.1.186", &sockaddr.sin_addr);

    if(connect(fd, (struct sockaddr*)&sockaddr, sizeof(struct sockaddr)) != 0)
    {
        newlog(get_time()) << "连接服务器失败" << endl;
        close(fd);
        return -1;
    }
    return fd;
}
//产生一个随机字符串
string random_generate(int len)
{
    string str;
    int i = 0;
    while(i++ < len)
    {
        int index = random() % 62;
        if(index < 10)
        {
            str.push_back('0' + index);
        }
        else if(index < 36)
        {
            str.push_back('a' + index - 10);
        }
        else
        {
            str.push_back('A' + index - 36);
        }
    }
    return str;
}
map<string, int> read_run_time()
{
    map<string, int> m;
    ifstream in;
    in.open("od_run_time", ios::in);
    assert(in.is_open());
    string line;
    while(getline(in, line))
    {
        auto vec = split(line, ",");
        if(vec.size() == 4)
        {
            auto ods = split(vec[0], " ");
            if(stationStrToInt.find(ods[0]) != stationStrToInt.end() &&
                    stationStrToInt.find(ods[1]) != stationStrToInt.end())
            {
                m[vec[0]] = atoi(vec[1].data());
            }
        }
    }
    auto ite = stationStrToInt.begin();
    while(ite != stationStrToInt.end())
    {
        auto iite = stationStrToInt.begin();
        while(iite != stationStrToInt.end())
        {
            if(ite->first == iite->first)
            {
                iite++;
                continue;
            }
            string od1 = ite->first + " " + iite->first;
            string od2 = iite->first + " " + ite->first;
            if(m.find(od1) == m.end() && m.find(od2) != m.end())
            {
                m[od1] = m[od2];
            }
            if(m.find(od2) == m.end() && m.find(od1) != m.end())
            {
                m[od2] = m[od1];
            }
            if(m.find(od2) == m.end() && m.find(od1) == m.end())
            {
                cout << od1 << endl;
            }
            iite++;
        }
        ite++;
    }
    newlog(get_time()) << "总的运行时间条数 " << m.size() << endl;
    return m;
}
int main()
{
    srand(time(0));
    read_station("station.txt");
    int stationSize = stationStrToInt.size();
    map<int, string> email_map;
    map<int, vector<Courier>> courier_map;  //每个车站的进站乘客
    map<int, vector<Courier>> arrive_map;

    //获取当天日期
    time_t begin_t = get_time();
    struct tm* tm = localtime(&begin_t);
    struct tm begin_time;
    memcpy(&begin_time, tm, sizeof(begin_time));

    //为每个站点创建工作人员
    newlog(get_time()) << "开始注册" << endl;
    int i = 0;
    while(i++ < stationSize)
    {
        if(i != 6 && i != 7 && i != 8 && i != 11 &&
                i != 13 && i != 14 && i != 15)
        {
            continue;
        }
        string name = random_generate(5);
        string email = random_generate(8) + "@qq.com";
        string telephone = "11111111111";
        string gender = "1";
        int srcIndex = i;
        int dstIndex = i;
        //发送注册信息
        string msg = "1name_" + name + " email_" + email + " telephone_"
            + telephone + " gender_" + gender + " src_" + to_string(srcIndex)
            + " dst_" + to_string(dstIndex);
        int fd = get_socket_fd();
        if(fd < 0)
        {
            i--;
            newlog(get_time()) << "套接字创建失败" << endl;
            sleep(5);
            continue;
        }
        send_all_message(fd, msg);
        string response = recv_all_message(fd);
        close(fd);
        //检查是否注册成功
        auto key_value = decodeRequese(response);
        if(key_value.find("email") == key_value.end())
        {
            i--;
            newlog(get_time()) << "车站 " << stationIntToStr[srcIndex] << " 注册失败" << endl;
            sleep(5);
            continue;
        }
        else
        {
            email_map[srcIndex] = email;
            courier_map[srcIndex] = vector<Courier>();
            arrive_map[srcIndex] = vector<Courier>();
        }

        //sleep(1);
    }
    //读取记录文件
    read_record(courier_map);

    //进行仿真
    int left_parcel = 0;
    i = 0;
    begin_time.tm_hour = EMUL_START_HOUR;
    begin_time.tm_min = begin_time.tm_sec = 0;
    newlog(mktime(&begin_time)) << "开始仿真" << endl;
    while(begin_time.tm_hour <= EMUL_END_HOUR + 3 || left_parcel > 0)
    {
        i++;
	time_t base_time = mktime(&begin_time);
	base_time += 1;
        tm = localtime(&base_time);
        memcpy(&begin_time, tm, sizeof(begin_time));
        if(i % 30 == 0)
        {
            //newlog(base_time) << "正在进行实验" << endl;
        }
        //遍历每个车站进行包裹投递和确认
        auto ite = courier_map.begin();
        while(ite != courier_map.end())
        {
            //检查每个车站的进站人员
            int index = ite->first;
            string current_station = stationIntToStr[index];
            auto& ins = ite->second;
            for(vector<Courier>::size_type j = 0; j < ins.size(); j++)
            {
                //遍历每个到达的乘客 检查是否可以投递
                auto courier = ins[j];
		tm = localtime(&courier.in_time);
		if(tm->tm_hour >= EMUL_END_HOUR)
		{
			continue;
		}
		
                if(courier.src != current_station)
                {
                    newlog(base_time) << "进站乘客错误" << endl;
                }
                if(courier.in_time <= base_time)
                {
                    string msg = "5email_" + email_map[index] +
                        " src_" + to_string(stationStrToInt[courier.src]) +
                        " dst_" + to_string(stationStrToInt[courier.dst]) +
			" time_" + to_string(courier.in_time);
                    int fd = get_socket_fd();
                    if(fd < 0)
                    {
                        newlog(base_time) << "套接字创建失败" << endl;
                        sleep(5);
                        continue;
                    }
                    send_all_message(fd, msg);
                    string response = recv_all_message(fd);
                    close(fd);
                    if(response.find("\n") != string::npos)
                    {
                        response = response.substr(0, response.find("\n"));
                    }
                    auto key_value = decodeRequese(response);
                    ins.erase(ins.begin() + j);
                    j--;
                    //有合适的包裹
                    if(key_value.find("parcel") != key_value.end() &&
                            key_value["parcel"].size() >= PARCEL_ID_LENGTH)
                    {
                        //设置乘客的出站时间和地点
                        int dstIndex = stationStrToInt[courier.dst];
                        if(arrive_map.find(dstIndex) == arrive_map.end())
                        {
                            arrive_map[dstIndex] = vector<Courier>();
                        }
                        courier.parcel = key_value["parcel"];
                        arrive_map[stationStrToInt[courier.dst]].push_back(courier);
			auto vec = split(key_value["parcel"], "-");
                        left_parcel += vec.size();
                        newlog(base_time) << "包裹 " << courier.parcel << " 将从 " <<
                            courier.src << " 送往 " << courier.dst << endl;
                    }
                }
            }
            //检查每个车站的出站人员
            auto& outs = arrive_map[index];
            for(vector<Courier>::size_type j = 0; j < outs.size(); j++)
            {
                auto courier = outs[j];
                if(courier.dst != current_station)
                {
                    newlog(base_time) << "出站乘客错误" << endl;
                }
                if(courier.out_time <= base_time)
                {
                    auto parcels = split(courier.parcel, "-");
                    for(auto parcel : parcels)
                    {
                        if(parcel.size() == PARCEL_ID_LENGTH)
                        {
                            string msg = "6parcel_" + parcel + 
                                " email_" + email_map[index] +
                                " time_" + to_string(courier.out_time) +
                                " station_" + to_string(index) +
				" time_" + to_string(courier.out_time); 
                            int fd = get_socket_fd();
                            if(fd < 0)
                            {
                                newlog(base_time) << "套接字创建失败" << endl;
                                sleep(5);
                                continue;
                            }
                            send_all_message(fd, msg);
                            string response = recv_all_message(fd);
                            if(response.find("\n") != string::npos)
                            {
                                response = response.substr(0, response.find("\n"));
                            }
                            close(fd);
                            auto key_value = decodeRequese(response);
                            if(key_value.find("parcel") != key_value.end() &&
                                    key_value["parcel"].size() == PARCEL_ID_LENGTH)
                            {
                                newlog(base_time) << "包裹 " << parcel << " 到达 "
                                    << courier.dst << endl;
                                left_parcel--;
                            }
                            else
                            {
                                newlog(base_time) << "包裹 " << parcel << " 投递失败 " << endl;
                            }
                        }
                    }
                    outs.erase(outs.begin() + j);
                    j--;
                }
            }
            ite++;
        }
    }
}

