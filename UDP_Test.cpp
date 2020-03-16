#pragma comment(lib, "ws2_32")

#include <iostream>
#include <winsock2.h>
#include "Stack.h"

#define SERVERPORT 9000
#define BUFSIZE 512

int main()
{
    Stack<int> *s;

    int retval;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 0;

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET)
        return 0;

    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = bind(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));

    if (retval == INVALID_SOCKET)
        return 0;

    SOCKADDR_IN clientaddr;
    int addrlen;
    char buf[BUFSIZE + 1];

    while (1)
    {
        addrlen = sizeof(clientaddr);
        retval = recvfrom(sock, buf, BUFSIZE, 0, (SOCKADDR*)&clientaddr, &addrlen);

        if (retval == SOCKET_ERROR)
            continue;


    }

    return 0;
}

