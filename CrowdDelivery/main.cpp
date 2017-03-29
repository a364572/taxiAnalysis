#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/signal.h>
#include<sys/time.h>
#include<errno.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<iostream>
#include<string>
#include<pthread.h>
#include "utils.h"
#include "commom.h"

using namespace std;

bool isRunning = true;
map<string, int> stationStrToInt;
map<int, string> stationIntToStr;
void handle_signal(int)
{
    log() << "准备退出程序" << endl;
    isRunning = false;
    exit_matlab();
}
//随机产生包裹，总个数 开始小时 结束小时 左闭右开 line:1/2
void* handle_input(void *)
{
    string line;
    while(getline(cin, line))
    {
        vector<string> res = split(line, string(" "));
        if(res[0][0] == 'a' && res.size() >= 3)
        {
            Parcel *parcel = new Parcel();
            parcel->src = res[1];
            parcel->dst = res[2]; 
            add_parcel(parcel, 0);
        }
        else if(res[0][0] == 'l')
        {
            if(res.size() == 2)
            {
                if(parcelInStation.find(res[1]) == parcelInStation.end())
                {
                    log() << "车站 " << res[1] << " 没有包裹" << endl;
                }
                else
                {
                    log() << "车站 " << res[1] << " 的包裹列表如下" << endl;
                    auto ite = parcelInStation[res[1]].begin();
                    while(ite != parcelInStation[res[1]].end())
                    {
                        auto parcel = *ite;
                        string line;
                        line.append("包裹ID: " + parcel->id + "\n");
                        line.append("\t包裹起点: " + parcel->src + "\n");
                        line.append("\t包裹终点: " + parcel->dst + "\n");
                        line.append("\t包裹投递中: " + string(parcel->isCarried ? "是" : "否") + "\n");
                        line.append("\t包裹当前站点: " + parcel->stations.back() + "\n");
                        line.append("\t包裹起始时间: " + to_string(parcel->times[0]));
                        cout << line << endl;
                        ite++;
                    }
                }
            }
            else if(res.size() == 3)
            {
                int cnt;
                string id;
                string src = res[1];
                string dst = res[2];
                get_parcel_count(src, dst, id, cnt);
                if(cnt == 0)
                {
                    log() << "没有从 " << src << " 到 " << dst << " 的包裹" << endl;
                }
                else
                {
                    log() << "从 " << src << " 到 " << dst << " 有 " << cnt << " 个包裹" << endl;
                }
            }
        }
    }
    return NULL;
}

int main(int argc, char** args)
{
    //log();
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock < 0)
    {
        log() << "创建socket失败" <<  endl;
        return -1;
    }
    struct sockaddr_in server;
    memset((char *)&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("119.29.32.99");
    server.sin_port = htons(9998);
    //连接云服务器
    if(connect(sock, (struct sockaddr*)&server, sizeof(struct sockaddr)) != 0)
    {
        log() << "连接tcloud失败" << endl;
        close(sock);
        return -1;
    }
    log() << "服务器启动成功" << endl;

    srand(time(NULL));
    read_station("station.txt");
    read_users("users");

    //提前产生包裹
    if(argc == 2)
    {
        int val = atoi(args[1]);
        if(val == 1 || val == 2)
        {
            generate_parcels(TOTAL_PARCEL_NUMBER, EMUL_START_HOUR, EMUL_END_HOUR, val);
        }
        string parcel_file = "parcels";
        read_parcels(parcel_file);
    }
    //启动matlab引擎
    init_matlab();

    struct sigaction action;
    memset((char *)&action, 0, sizeof(action));
    action.sa_handler = handle_signal;
    action.sa_flags = SA_NOMASK;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    char tmp[1024];
    string msg;
    while(isRunning) 
    {
        memset(tmp, 0, sizeof(tmp));
        int len = recv(sock, tmp, sizeof(tmp) - 1, 0);
        if(len <= 0)
        {
            log() << "消息接收失败" << endl;
            isRunning = false;
        }
        else
        {
            msg += tmp;
            string::size_type pos = msg.find("\n");
            while(pos != string::npos)
            {
                string str = msg.substr(0, pos);
                handle_message_in(sock, str);
                msg = msg.substr(pos + 1);
                pos = msg.find("\n");
            }
        }
    }
    close(sock);
    write_parcels("parcels");
    write_users("users");
    exit_matlab();
}

