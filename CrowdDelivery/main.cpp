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

#define MAX_EPOLL_EVENT_COUNT 10

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
//随机产生包裹，总个数 开始小时 结束小时 左闭右开
void generate_parcels(int num, int beginHour, int endHour)
{
    int cnt = 0;
    int totalStation = stationIntToStr.size();
    int total_minutes = (endHour - beginHour) * 60;
    //获取当天日期
    time_t now = get_time();
    struct tm timeBegin;
    struct tm* tm = localtime(&now);
    memcpy(&timeBegin, tm, sizeof(timeBegin));
    timeBegin.tm_sec = timeBegin.tm_min = tm->tm_hour = 0;

    while(cnt < num)
    {

        int nextSrc = random() % totalStation + 1;
        int nextDst = nextSrc;
        while(nextDst == nextSrc)
        {
            nextDst = random() % totalStation + 1;
        }
        Parcel *parcel = new Parcel();
        parcel->src = stationIntToStr[nextSrc];
        parcel->dst = stationIntToStr[nextDst];
        int nextMinute = random() % total_minutes;
        timeBegin.tm_hour = beginHour + nextMinute / 60;
        timeBegin.tm_min = nextMinute % 60;
        now = mktime(&timeBegin);
        //设置包裹的起始时间 起始站点
        parcel->start_time = now;
        parcel->times.push_back(now);
        parcel->stations.push_back(parcel->src);
        add_parcel(parcel, 1);
        cnt++;
    }
    log() << "共产生了 " << cnt << " 个包裹" << endl;
}
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
                        line.append("\t包裹起始时间: " + toString(parcel->times[0]));
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
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock < 0)
    {
        log() << "创建socket失败" << endl;
        return -1;
    }
    int value = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)); 

    struct sockaddr_in addr;
    memset((char *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    addr.sin_port = htons(SERVER_PORT);

    if(bind(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr)) != 0)
    {
        log() << "绑定套接字失败,请稍后再试" << endl;
        return -1;
    }
    log() << "服务器启动成功" << endl;

    listen(sock, 5);
    srand(time(NULL));
    read_station("station.txt");
    read_parcels("parcels");
    read_users("users");

    struct sigaction action;
    memset((char *)&action, 0, sizeof(action));
    action.sa_handler = handle_signal;
    action.sa_flags = SA_NOMASK;
    sigaction(SIGINT, &action, NULL);

    pthread_t pth;
    pthread_create(&pth, NULL, handle_input, NULL);
    int new_sock;
    struct sockaddr_in client;
    socklen_t sock_length;
    sock_length = sizeof(struct sockaddr);

    //提前产生包裹
    if(argc == 2)
    {
        generate_parcels(100, EMUL_START_HOUR, EMUL_END_HOUR);
    }
    //启动matlab引擎
    init_matlab();

    while(isRunning) 
    {
        new_sock = accept(sock, (struct sockaddr*)&client, &sock_length);
        if(new_sock < 0)
        {
           break;
        } 
        handle_message_in(new_sock);
        close(new_sock);
    }
    write_parcels("parcels");
    write_users("users");
}

