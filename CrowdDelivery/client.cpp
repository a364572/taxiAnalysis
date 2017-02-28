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
#include "utils.h"
#include "commom.h"

using namespace std;
#define COURIER_NUMBER 10       //每个车站的快递员数

map<string, int> stationStrToInt;
map<int, string> stationIntToStr;
struct Courier 
{
    time_t in_time;
    string src;
    string dst;
    string parcel;
};

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

    if(connect(fd, (struct sockaddr*)&sockaddr, sizeof(struct sockaddr)) != 0)
    {
        log() << "连接服务器失败" << endl;
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
int main()
{
    srand(time(0));
    read_station("station.txt");
    int stationSize = stationStrToInt.size();
    map<string, int> run_time_map;
    map<int, string> email_map;
    map<int, vector<Courier>> courier_map;
    map<int, vector<Courier>> arrive_map;

    //获取当天日期
    time_t begin_t = get_time();
    struct tm* tm = localtime(&begin_t);
    struct tm begin_time;
    memcpy(&begin_time, tm, sizeof(begin_time));

    //为每个站点创建工作人员
    log() << "开始注册" << endl;
    int i = 0;
    while(i++ < stationSize)
    {
        string name = random_generate(5);
        string email = random_generate(8) + "@qq.com";
        string telephone = "11111111111";
        string gender = "男";
        int srcIndex = i;
        int dstIndex = random() % stationSize + 1;
        //发送注册信息
        string msg = "1name_" + name + " email_" + email + " telephone_"
            + telephone + " gender_" + gender + " src_" + stationIntToStr[srcIndex]
            + " dst_" + stationIntToStr[dstIndex];
        int fd = get_socket_fd();
        if(fd < 0)
        {
            i--;
            log() << "套接字创建失败" << endl;
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
            log() << "车站 " << stationIntToStr[srcIndex] << " 注册失败" << endl;
            sleep(5);
            continue;
        }
        else
        {
            email_map[srcIndex] = email;
            courier_map[srcIndex] = vector<Courier>();
            arrive_map[srcIndex] = vector<Courier>();
        }
        if(i == 1)
        {
            return -1;
        }

        //模拟有乘客进站 乘客进站时间随机 目的地随机
        int interval = (EMUL_END_HOUR - EMUL_START_HOUR) * 60;
        begin_time.tm_min = begin_time.tm_sec = 0;

        int j = 0;
        int eslape_time = 0;
        while(j++ < COURIER_NUMBER)
        {
            dstIndex = random() % stationSize + 1;
            string dst = stationIntToStr[dstIndex];
            int in_time = random() % ((interval - eslape_time) / 2);
            eslape_time += in_time;

            begin_time.tm_hour = EMUL_START_HOUR + eslape_time / 60;
            begin_time.tm_min = eslape_time % 60;
            Courier courier;
            courier.in_time = mktime(&begin_time);
            courier.src = stationIntToStr[srcIndex];
            courier.dst = stationIntToStr[dstIndex];
            courier.parcel = "0";
            courier_map[srcIndex].push_back(courier);
        }
        sleep(1);
    }

    //进行仿真
    begin_time.tm_hour = EMUL_START_HOUR;
    begin_time.tm_min = begin_time.tm_sec = 0;
    while(begin_time.tm_hour < EMUL_END_HOUR)
    {
        begin_t = get_time();
        tm = localtime(&begin_t);
        memcpy(&begin_time, tm, sizeof(begin_time));
        //遍历每个车站进行包裹投递
        auto ite = courier_map.begin();
        while(ite != courier_map.end())
        {
            //检查每个车站的进站人员
            int index = ite->first;
            auto& ins = ite->second;
            for(vector<Courier>::size_type j = 0; j < ins.size(); j++)
            {
                auto& courier = ins[j];
                //遍历每个到达的乘客 检查是否可以投递
                if(courier.in_time <= begin_t)
                {
                    string msg = "5email_" + email_map[index] +
                        " src_" + courier.src +
                        " dst_" + courier.dst;
                    int fd = get_socket_fd();
                    if(fd < 0)
                    {
                        log() << "套接字创建失败" << endl;
                        sleep(5);
                        continue;
                    }
                    send_all_message(fd, msg);
                    string response = recv_all_message(fd);
                    close(fd);
                    auto key_value = decodeRequese(response);
                    //有合适的包裹
                    if(key_value.find("parcel") != key_value.end() &&
                            key_value["parcel"].size() == PARCEL_ID_LENGTH)
                    {
                        ins.erase(ins.begin() + j);
                        j--;
                        //设置乘客的出站时间和地点
                        string od = courier.src + " " + courier.dst;
                        courier.in_time += run_time_map[od];
                        courier.parcel = key_value["parcel"];
                        arrive_map[stationStrToInt[courier.dst]].push_back(courier);
                        log() << "包裹 " << courier.parcel << " 将从 " <<
                            courier.src << " 送往 " << courier.dst << endl;
                    }
                }
                else
                {
                    break;
                }
            }
            //检查每个车站的出站人员
            auto& outs = arrive_map[index];
            for(vector<Courier>::size_type j = 0; j < outs.size(); j++)
            {
                auto& courier = outs[j];
                if(courier.in_time <= begin_t)
                {
                    string msg = "6parcel_" + courier.parcel + 
                        " email_" + email_map[index] +
                        " time_0 station_" + courier.dst; 
                    int fd = get_socket_fd();
                    if(fd < 0)
                    {
                        log() << "套接字创建失败" << endl;
                        sleep(5);
                        continue;
                    }
                    send_all_message(fd, msg);
                    string response = recv_all_message(fd);
                    close(fd);
                    auto key_value = decodeRequese(response);
                    if(key_value.find("parcel") != key_value.end() &&
                            key_value["parcel"].size() == PARCEL_ID_LENGTH)
                    {
                        outs.erase(outs.begin() + j);
                        j--;
                        log() << "包裹 " << courier.parcel << " 到达 "
                           << courier.dst << endl;
                    }
                    else
                    {
                        log() << "包裹 " << courier.parcel << " 投递失败 " << endl;
                    }
                }
            }
            ite++;
        }
        sleep(1);
    }
}

