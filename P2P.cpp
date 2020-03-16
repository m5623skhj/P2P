#include "PreCompile.h"
#include "ServerCommon.h"
#include "P2PSerializeBuffer.h"
#include "P2P.h"

CP2P::CP2P() : m_UserNumber(0), m_Socket(INVALID_SOCKET)
{

}

CP2P::~CP2P()
{

}

bool CP2P::Start()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa))
	{
		WriteError(WSAGetLastError(), SERVER_ERR::WSASTARTUP_ERR);
		return false;
	}

	m_Socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_Socket == INVALID_SOCKET)
	{
		WriteError(WSAGetLastError(), SERVER_ERR::LISTEN_SOCKET_ERR);
		return false;
	}

	timeBeginPeriod(1);

	m_pOtherUserMemmoryPool = new CTLSMemoryPool<st_OtherUserInfo>(1, false);

	return true;
}

void CP2P::Stop()
{
	closesocket(m_Socket);

	WSACleanup();
}

void CP2P::HolePunching(const char *IP, const WORD Port)
{
	if (IP == nullptr || Port == 65535)
		return;
	
	// 특수한 패킷을 하나 보내어 이 패킷이 접속 패킷을 알림
	CP2PSerializationBuf& HolePuchingPacket = *CP2PSerializationBuf::Alloc();

	HolePuchingPacket.m_pSerializeBuffer[df_IS_CONNECT_LOCATION] = 1;

	SendPacket(IP, Port, &HolePuchingPacket);
}

void CP2P::SendPacket(UINT64 UserID, CP2PSerializationBuf* pPacket)
{
	
}

void CP2P::SendPacket(const char *IP, const WORD Port, CP2PSerializationBuf* pPacket)
{
	
}

void CP2P::ConnectOtherUser(const char* IP[3], const WORD Port[3])
{
	if (IP[0] == NULL || Port[0] == 65535)
		return;

	UINT64 UserNumber = InterlockedIncrement(&m_UserNumber);

	st_OtherUserInfo &NewOtherUser = *m_pOtherUserMemmoryPool->Alloc();
	
	NewOtherUser.NotRecv = 0;
	NewOtherUser.IsConnectUser = false;
	NewOtherUser.UserNumber = UserNumber;

	for (int i = 1; i < 3; ++i)
	{
		if (IP[i] == nullptr || Port[i] == 65535)
			break;

		st_ConnectTarget Target;
		memcpy_s(Target.IP, sizeof(Target.IP), IP[i], sizeof(Target.IP));
		Target.Port = Port[i];

		NewOtherUser.ConnectTargetStack.push(Target);
	}

	// 이후 락 걸어야될듯 함
	m_OtherUserMap.insert({ UserNumber, &NewOtherUser });

	// 이후 락 걸어야될듯 함
	m_UserFindMap.insert({ NewOtherUser.RecvOverlapped, UserNumber });
	m_UserFindMap.insert({ NewOtherUser.SendOverlapped, UserNumber });

	HolePunching(IP[0], Port[0]);
}

UINT CP2P::Worker()
{
	// 받아온 패킷이 접속 패킷일 경우가 존재하기 때문에
	// HolePuchingPacket.m_pSerializeBuffer[df_IS_CONNECT_LOCATION] 가 1인지 확인해보고
	// 그에 따른 분기를 나눌것

	CP2PSerializationBuf::ChunkFreeForcibly();
	return 0;
}

void CP2P::CheckOtherUserAlive()
{
	
}

void CP2P::UserErase()
{
	
}

char CP2P::RecvPost()
{


	return 0;
}

char CP2P::SendPost()
{


	return 0;
}

bool CP2P::P2POptionParsing(const WCHAR* szOptionFileName)
{


	return true;
}

void CP2P::WriteError(int WindowErr, int UserErr)
{
	st_Error Error;
	Error.GetLastErr = WindowErr;
	Error.ServerErr = UserErr;
	OnError(&Error);
}