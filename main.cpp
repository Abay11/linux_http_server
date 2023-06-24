#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

// Usual socket headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <arpa/inet.h>

#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>

#define SIZE 1024
#define BACKLOG 10  // Passed to listen()

using namespace std;

#define LOG(msg) \
	{ \
        std::lock_guard guard(m_logfileMutex); \
        m_logfile << "[TID=" << std::this_thread::get_id() << "] " << __FILE__ << "(" << __LINE__ << "):" << msg << std::endl; \
	}

class Server
{
public:
    Server(std::string ip, uint16_t port, std::string dir)
    {
        m_logfile.open("final.log", ios::out);
	    LOG(ip << " " << port << " " << dir << " Ok\n");
    }

    void startServerMain(std::string host, uint16_t port, std::string);

private:
    fstream m_logfile;
    std::mutex m_logfileMutex; 
};

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

    if (daemon(1, 1))
    {
        return EXIT_FAILURE;
    }

    Server s(ip, port, dir);

    s.startServerMain(ip, port, dir);

    return EXIT_SUCCESS;
}

void Server::startServerMain(std::string host, uint16_t port, std::string)
{
    // Socket setup: creates an endpoint for communication, returns a descriptor
    // -----------------------------------------------------------------------------------------------------------------
    int serverSocket = socket(
        AF_INET,      // Domain: specifies protocol family
        SOCK_STREAM,  // Type: specifies communication semantics
        0             // Protocol: 0 because there is a single protocol for the specified family
    );

    // Construct local address structure
    // -----------------------------------------------------------------------------------------------------------------
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(host.c_str()); //htonl(host.c_str());//

    // Bind socket to local address
    // -----------------------------------------------------------------------------------------------------------------
    // bind() assigns the address specified by serverAddress to the socket
    // referred to by the file descriptor serverSocket.
    bind(
        serverSocket,                         // file descriptor referring to a socket
        (struct sockaddr *) &serverAddress,   // Address to be assigned to the socket
        sizeof(serverAddress)                 // Size (bytes) of the address structure
    );

    // Mark socket to listen for incoming connections
    // -----------------------------------------------------------------------------------------------------------------
    int listening = listen(serverSocket, BACKLOG);
    if (listening < 0) {
        //LOG("Error: The server is not listening.\n");
        return;
    }

    //report(&serverAddress);     // Custom report function
    //setHttpHeader(httpHeader);  // Custom function to set header
    int clientSocket;

        // Wait for a connection, create a connected socket if a connection is pending
    // -----------------------------------------------------------------------------------------------------------------
    while(1) {
        clientSocket = accept(serverSocket, NULL, NULL);
        //send(clientSocket, httpHeader, sizeof(httpHeader), 0);
        close(clientSocket);
    }
}