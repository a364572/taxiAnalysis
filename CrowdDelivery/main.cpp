#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<sys/time.h>
#include<string.h>
#include<stdio.h>
#include<iostream>
#include<string>
#include<pthread.h>
#include "utils.h"

#define SERVER_IP "192.168.1.186"
#define SERVER_PORT 9999
#define MAX_EPOLL_EVENT_COUNT 10

using namespace std;

void* handle_input(void *)
{
    string line;
    while(getline(cin, line))
    {
        vector<string> res = split(line, string(" "));
        if(res[0][0] == 'a' && res.size() == 3)
        {
            Parcel *parcel = new Parcel();
            parcel->src = res[1];
            parcel->dst = res[2]; 
            add_parcel(parcel);
        }
    }
    return NULL;
}

int main()
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
    read_station("station.txt");

    pthread_t pth;
    pthread_create(&pth, NULL, handle_input, NULL);
    int new_sock;
    struct sockaddr_in client;
    socklen_t sock_length;
    sock_length = sizeof(struct sockaddr);
    while((new_sock = accept(sock, (struct sockaddr*)&client, &sock_length)) > 0)
    {
        handle_message_in(new_sock);
        close(new_sock);
    }
}

