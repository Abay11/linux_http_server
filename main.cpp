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
#include <functional>
#include <cstring>
#include <sstream>
#include <iterator>

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

    void startServerMain(std::string host, uint16_t port, const std::string& dir);

private:
    void readClientRequest(int socket);

    fstream m_logfile;
    std::mutex m_logfileMutex; 

    std::string m_dir;
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

void Server::startServerMain(std::string host, uint16_t port, const std::string& dir)
{
    m_dir = dir;

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
        LOG("Error: The server is not listening.\n");
        return;
    }

    //report(&serverAddress);     // Custom report function
    //setHttpHeader(httpHeader);  // Custom function to set header
    int clientFd;

    // Wait for a connection, create a connected socket if a connection is pending
    // -----------------------------------------------------------------------------------------------------------------
    while(1) {
        struct sockaddr client_addr;
        socklen_t address_len;
        clientFd = accept(serverSocket, (struct sockaddr *)&client_addr, &address_len);
        if (clientFd < 0)
        {
            LOG("accept() error");
        }
        else
        {
            char str[INET_ADDRSTRLEN];
            struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&client_addr;
            struct in_addr ipAddr = pV4Addr->sin_addr;
            inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );

            LOG("A new client with fd=" << clientFd << " has addr " << str << ":" << pV4Addr->sin_port);

            std::thread(std::bind(&Server::readClientRequest, this, clientFd)).detach();
        }
    }
}

void Server::readClientRequest(int clientFd)
{
    LOG("start processing a client with socket " << clientFd);
    int rcvd;
    auto* buf = new char[65535];
    rcvd=recv(clientFd, buf, 65535, 0);

    if (rcvd<0)    // receive error
    {
        LOG("recv() error\n");
    }
    else if (rcvd==0)    // receive socket closed
    {
        LOG("Client disconnected unexpectenly.\n");
    }
    else    // message received
    {
        buf[rcvd] = '\0';

        LOG("recv a request from the client[" << clientFd << "] '\n" << buf << "'");

        auto method = strtok(buf,  " \t\r\n");
        char* tmpUri    = strtok(NULL, " \t");
        std::string uri = tmpUri[0] == '/'
            ? (std::string(tmpUri + 1, strlen(tmpUri) - 1))
            : (std::string(tmpUri, strlen(tmpUri)));

        LOG("method determined as " << method << " and requested file " << uri);

        std::stringstream httpHeaderResponse;

        std::ifstream file(m_dir + uri, std::ios::binary);
        if (!file.is_open()) {
            LOG("Failed to open a file: " << m_dir + uri); 

            const std::string content = "The requested file was not found on this server.";
            httpHeaderResponse << "HTTP/1.0 404 NOT FOUND\r\n"
                << "Content-Type: text/html\r\n"
                << "Content-Length: 0\r\n\r\n";
        }
        else
        {
            httpHeaderResponse << "HTTP/1.0 200 OK\r\n"
                << "Content-Type: text/html\r\n";
        }

        httpHeaderResponse.seekg(0, std::ios::end);
        auto len = httpHeaderResponse.tellg();
        httpHeaderResponse.seekg(0, std::ios::beg);
        httpHeaderResponse.read(buf, len);
        LOG("sending buf: " << buf);
        if (send(clientFd, buf, len, 0) < 0) {
            LOG("Failed to send data");
        }
        else
        {
            LOG("Write response with len: " << len);
        }

        if (file.is_open())
        {
            file.seekg(0, std::ios::end);
            int size = file.tellg();
            file.seekg(0, std::ios::beg);
            file.read(buf, size);
            file.close();

            httpHeaderResponse.str("");
            httpHeaderResponse.clear();
            httpHeaderResponse << "Content-Length: " << size << "\r\n\r\n";
            std::string tmp = httpHeaderResponse.str();
            send(clientFd, tmp.data(), tmp.size(), 0);
            
            send(clientFd, buf, size, 0);
        }
    }

    delete[] buf;
    //Closing SOCKET
    shutdown(clientFd, SHUT_RDWR);         //All further send and recieve operations are DISABLED...
    close(clientFd);
}