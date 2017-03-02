#include "commom.h"
#include "utils.h"
#include<string.h>
#include<sstream>
#include<fstream>
#include<unistd.h>
#include<sys/socket.h>
using namespace std;

ostream& log()
{
    time_t now = get_time();
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

void send_all_message(int fd, string& msg)
{
    //int len = msg.size();
    //int sent = 0;
    //int res = 0;
    //while(len > sent && 
    //        (res = send(fd, msg.data() + sent , len - sent, 0) > 0))
    //{
    //    sent += res;
    //}
    //发送完数据后发送终止符
    int sent = send(fd, msg.data(), msg.size(), 0);
    shutdown(fd, SHUT_WR);
    //log() << msg << " 写长度 " << sent << endl;
}

string recv_all_message(int fd)
{
    char tmp[1024];
    memset(tmp, 0, sizeof(tmp));
    //int total_length = 0;
    //int length = 0;
    //while((length = recv(fd, tmp + total_length, sizeof(tmp) - total_length - 1, 0)) > 0)
    //{
    //    total_length += length;
    //    log() << length << endl;
    //}

    //shutdown(fd, SHUT_RD);
    //if(total_length <= 0)
    //{
    //    return "";
    //}
    int total_length = recv(fd, tmp, sizeof(tmp), 0);
    shutdown(fd, SHUT_RD);
    //log() << tmp << " 读长度 " << total_length << endl;
    return tmp;
}
//返回系统当前时间，如果是仿真，时间走得更快
time_t get_time()
{
    static time_t begin = 0;
    static time_t base = 0;
    if(begin == 0)
    {
        begin = time(0);

        struct tm* tm = localtime(&begin);
        tm->tm_hour = EMUL_START_HOUR;
        tm->tm_min = tm->tm_sec = 0;
        base = mktime(tm);

        return begin;
    }
    time_t now = time(0);
    return base + (now - begin) * EMULATION_SPEED;
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
        vector<string> res = split(line, " ");
        int index = atoi(res[1].data());
        stationStrToInt[res[0]] = index;
        stationIntToStr[index] = res[0];
    }
    in.close();
}
