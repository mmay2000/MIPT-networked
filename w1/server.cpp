#include <sys/types.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <cstring>
#include <cstdio>
#include "socket_tools.h"
#include <iostream>
#include <string>
#include <thread>

void listenSocket(int sfd, int aliveCounter) 
{
    while (true) 
    {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sfd, &readSet);

        timeval timeout = { 0, 100000 }; // 100 ms
        select(sfd + 1, &readSet, NULL, NULL, &timeout);


        if (FD_ISSET(sfd, &readSet))
        {
            constexpr size_t buf_size = 1000;
            static char buffer[buf_size];
            memset(buffer, 0, buf_size);

            size_t numBytes = recvfrom(sfd, buffer, buf_size - 1, 0, nullptr, nullptr);
            if (numBytes > 0)
                if (buffer[0] != 'A')
                    printf("%s\nAlive massage received: %d\n", buffer, aliveCounter); // assume that buffer is a string
                else
                    aliveCounter++;
        }
    }
}



int main(int argc, const char** argv)
{

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) 
    {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }
    const char* port = "2023", * recv_port = "2022";
    addrinfo resAddrInfo;

    int sfd = create_dgram_socket(nullptr, port, nullptr);
    int sfd_recv = create_dgram_socket("localhost", recv_port, &resAddrInfo);

    printf("listening!\n");
    int aliveCounter = 0;
    std::thread thr(listenSocket, sfd, aliveCounter);
    thr.detach();

    std::string input;
    while (true) 
    {
        printf(">");
        std::getline(std::cin, input);
        size_t res = sendto(sfd_recv, input.c_str(), input.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
        if (res == -1) 
        {
            std::cout << strerror(errno) << std::endl;
        }
    }
    WSACleanup();
  return 0;
}
