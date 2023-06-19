#include <getopt.h>

#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>

using namespace std;

#define LOG(msg) \
	std::lock_guard<std::mutex> guard(logfileMutex); \
    logfile << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl 

int main(int argc, char **argv)
{
	std::mutex logfileMutex;
	fstream logfile("final.log", ios::out);

	int r = 0;
	string ip;
	string dir;
	uint16_t port = 0;
	while((r = getopt(argc, argv, "h:d:p:")) != -1)
	{
		switch(r)
		{
		case 'h': ip = optarg; break;
		case 'd': dir = optarg; break;
		case 'p': port = stoi(optarg); break;
		default: break;
		}
	}

	LOG(ip << " " << port << " " << dir << " Ok\n");

	return 0;
}