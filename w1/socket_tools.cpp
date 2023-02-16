#include <winsock2.h>
#include <WS2tcpip.h>
#include <fcntl.h>
#include <cstring>
#include <stdio.h>

#include "socket_tools.h"

// Adaptation of linux man page: https://linux.die.net/man/3/getaddrinfo
static int get_dgram_socket(addrinfo *addr, bool should_bind, addrinfo *res_addr)
{
    WSADATA wsaData;
   int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }
  for (addrinfo *ptr = addr; ptr != nullptr; ptr = ptr->ai_next)
  {
    if (ptr->ai_family != AF_INET || ptr->ai_socktype != SOCK_DGRAM || ptr->ai_protocol != IPPROTO_UDP)
      continue;
    int sfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (sfd == -1)
      continue;

    u_long iMode = 1;
    ioctlsocket(sfd, FIONBIO, &iMode);

    bool trueVal = true;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*) &trueVal, sizeof(int));

    if (res_addr)
      *res_addr = *ptr;
    if (!should_bind)
    {
        WSACleanup();
        return sfd;
    }

    if (bind(sfd, ptr->ai_addr, ptr->ai_addrlen) == 0)
    {
        WSACleanup();
        return sfd;
    }

    closesocket(sfd);
    WSACleanup();
  }
  WSACleanup();
  return -1;
}

int create_dgram_socket(const char *address, const char *port, addrinfo *res_addr)
{
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }
  addrinfo hints;
  memset(&hints, 0, sizeof(addrinfo));

  bool isListener = !address;

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  if (isListener)
    hints.ai_flags = AI_PASSIVE;

  addrinfo *result = nullptr;
  if (int err = getaddrinfo(address, port, &hints, &result) != 0)
  {
      printf("getaddrinfo failed with error: %d\n", err);
      WSACleanup();
      return 1;
  }

  int sfd = get_dgram_socket(result, isListener, res_addr);

  //printf("%d", sfd);
  //freeaddrinfo(result);
  WSACleanup();
  return sfd;
}

