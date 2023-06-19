#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>

using namespace std;


namespace
{
//fstream logfile("final.log", ios::out);
}


#define LOG(msg) \
	{ \
    fstream("final.log", ios::out) << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl; \
	}

/*
 * daemonize.c
 * This example daemonizes a process, writes a few log messages,
 * sleeps 20 seconds and terminates afterwards.
 */


static void skeleton_daemon()
{
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    //TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }

    /* Open the log file */
    LOG("firstdaemon " << LOG_PID << " " << LOG_DAEMON);
}

int main(int argc, char **argv)
{
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

	skeleton_daemon();

	while (1)
    {
        //TODO: Insert daemon code here.
        //LOG(LOG_NOTICE << " First daemon started.");
        sleep (10);
        break;
    }

    //LOG(LOG_NOTICE << " First daemon terminated.");

    return EXIT_SUCCESS;
}