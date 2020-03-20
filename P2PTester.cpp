#include "P2PTester.h"
#include "P2PSerializeBuffer.h"


P2PTester::P2PTester()
{

}

P2PTester::~P2PTester()
{

}

void P2PTester::StartP2P()
{
	Start(L"Test.txt");
	//WCHAR IP[3][16] = { L"127.0.0.1", L"", L"" };
	//WORD Port[3];

	//Port[0] = 5000;

	//ConnectOtherUser((const WCHAR**)IP, Port, 1);
}

void P2PTester::StopP2P()
{
	Stop();
}

void P2PTester::ShootTarget()
{
	//CP2PSerializationBuf &buf = *CP2PSerializationBuf::Alloc();
	//SendPacket(1, &buf);
	//CP2PSerializationBuf::Free(&buf);
}

void P2PTester::OtherUserConnected(UINT64 UserID)
{
	printf("A");
}

void P2PTester::OtherUserDisConnected(UINT64 UserID)
{
	printf("B");

}

void P2PTester::OnRecv(UINT64 UserID, CP2PSerializationBuf* pPacket)
{
	printf("C");

}

void P2PTester::OnSend(UINT64 UserID, int SendSize)
{
	printf("D");

}

void P2PTester::OnWorkerThreadBegin()
{
	printf("E");

}

void P2PTester::OnWorkerThreadEnd()
{
	printf("F");

}

void P2PTester::OnConnectOtherUserFail(UINT64 UserID)
{
	printf("G");

}

void P2PTester::OnError(st_Error* OutError)
{
	printf("H");

}

void P2PTester::GQCSFailed(int LastError)
{
	printf("I");

}

