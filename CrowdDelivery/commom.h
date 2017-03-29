#include<map>
#include<string>
#include<vector>
#include<iostream>
#include<sys/time.h>

std::ostream& log();

std::string recv_all_message(int fd);
void send_all_message(int fd, std::string&);
void send_all_message(int fd, std::string, int);

void read_station(std::string);
std::map<std::string, std::string> decodeRequese(std::string);
std::vector<std::string> split(std::string &line, std::string delim);

time_t get_time();
