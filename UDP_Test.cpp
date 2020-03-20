#include "P2PTester.h"

int main()
{
    P2PTester p2pTester;

    p2pTester.StartP2P();
    while (1)
    {
        Sleep(1000);
        p2pTester.ShootTarget();
    }

    //char IP[16] = "127.0.0.1";
    //WORD Port = 7777;

    //SOCKADDR_IN addr;

    //ZeroMemory(&addr, sizeof(addr));
    //addr.sin_family = AF_INET;
    //addr.sin_addr.S_un.S_addr = inet_addr(IP);
    //addr.sin_port = htons(Port);

    //sockaddr* paddr;
    //paddr = (sockaddr*)&addr;

    return 0;
}

