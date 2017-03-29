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
#include<sys/epoll.h>

#define MAX_EPOLL_EVENT_COUNT 10
#define TOTAL_PARCEL_NUMBER 15

using namespace std;

bool isRunning = true;
void handle_signal(int)
{
    cout << "准备退出程序" << endl;
    isRunning = false;
}

int accept_socket(int sock)
{
    if(sock < 0)
    {
        return -1;
    }
    struct sockaddr_in client;
    socklen_t len = sizeof(struct sockaddr);

    return accept(sock, (struct sockaddr*)&client, &len);
}
//创建两个监听端口 9999用于接收用户连接 9998用于接收vlab连接
//每次从9999收到数据 直接转发给9998 然后将9998返回的结果发给9999
int main()
{
    int sock_user = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int sock_vlab = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock_user < 0 || sock_vlab < 0)
    {
        cout << "创建socket失败" << endl;
        return -1;
    }
    int value = 1;
    setsockopt(sock_user, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)); 
    value = 1;
    setsockopt(sock_vlab, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)); 

    struct sockaddr_in addr;
    //绑定9999套接字
    memset((char *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    addr.sin_port = htons(9999);
    if(bind(sock_user, (struct sockaddr*)&addr, sizeof(struct sockaddr)) != 0)
    {
        cout << "绑定9999套接字失败,请稍后再试" << endl;
        return -1;
    }
    //绑定9998套接字
    memset((char *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    addr.sin_port = htons(9998);
    if(bind(sock_vlab, (struct sockaddr*)&addr, sizeof(struct sockaddr)) != 0)
    {
        cout << "绑定9998套接字失败,请稍后再试" << endl;
        return -1;
    }
    cout << "服务器启动成功" << endl;

    listen(sock_user, 5);
    listen(sock_vlab, 5);
    struct sigaction action;
    memset((char *)&action, 0, sizeof(action));
    action.sa_handler = handle_signal;
    action.sa_flags = SA_NOMASK;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    int epoll_fd = epoll_create(5);
    if(epoll_fd < 0)
    {
        cout << "epoll 创建失败" << endl;
        return -1;
    }

    struct epoll_event event;
    event.data.fd = sock_user;
    event.events = EPOLLIN;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_user, &event) != 0)
    {
        cout << "epoll add 失败" << endl;
        return -1;
    }
    event.data.fd = sock_vlab;
    event.events = EPOLLIN;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_vlab, &event) != 0)
    {
        cout << "epoll add 失败" << endl;
        return -1;
    }

    char tmp[1024];
    int proxy_fd = -1;
    string msg;
    struct epoll_event events[MAX_EPOLL_EVENT_COUNT];
    while(isRunning)
    {
        int cnt = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENT_COUNT, -1);
        if(cnt < 0)
        {
            cout << "epoll wait 失败" << endl;
            break;
        }
        for(int i = 0; i < cnt; i++)
        {
            if(events[i].data.fd == sock_user && events[i].events == EPOLLIN)
            {
                int new_fd = accept_socket(sock_user);
                if(new_fd >= 0)
                {
                    //接收数据并转发 
                    if(proxy_fd >= 0)
                    {
                        memset(tmp, 0, sizeof(tmp));
                        int len = recv(new_fd, tmp, sizeof(tmp) - 1, 0);
                        if(len > 0 && proxy_fd >= 0)
                        {
                            string req = tmp;
                            req += "\n";

                            //把数据发给vlab
                            send(proxy_fd, req.data(), req.size(), 0);
                            memset(tmp, 0, sizeof(tmp));
                            if(recv(proxy_fd, tmp, sizeof(tmp) - 1, 0) <= 0)
                            {
                                cout << "vlab数据读取失败" << endl;
                                close(proxy_fd);
                                proxy_fd = -1;
                            }

                            msg += tmp;
                            string::size_type index = msg.find("\n");
                            while(index == string::npos)
                            {
                                memset(tmp, 0, sizeof(tmp));
                                if(recv(proxy_fd, tmp, sizeof(tmp) - 1, 0) <= 0)
                                {
                                    cout << "vlab数据读取失败" << endl;
                                    close(proxy_fd);
                                    proxy_fd = -1;
                                    break;
                                }
                                msg += tmp;
                                index = msg.find("\n");
                            }
                            string res = msg.substr(0, index);
                            msg = msg.substr(index + 1);
                            
                            send(new_fd, res.data(), res.size(), 0);
                        }
                    }
                    close(new_fd);
                }
            }
            if(events[i].data.fd == sock_vlab && events[i].events == EPOLLIN)
            {
                if(proxy_fd >= 0)
                {
                    close(proxy_fd);
                }
                proxy_fd = accept_socket(sock_vlab);
            }
        }
    }
}

