#pragma once
#include "PreCompile.h"
#include <map>
#include <stack>

class CP2PSerializationBuf;

class CP2P
{
public :
	CP2P();
	~CP2P();

protected :
	bool Start();
	void Stop();

	virtual void OtherUserConnected(UINT64 UserID) = 0;
	virtual void OtherUserDisConnected(UINT64 UserID) = 0;

	virtual void OnError(st_Error* OutError) = 0;

	virtual void OnRecv(UINT64 UserID, CP2PSerializationBuf *pPacket) = 0;
	virtual void OnRecv(UINT64 UserID, int SendSize) = 0;

	void SendPacket(UINT64 UserID, CP2PSerializationBuf*pPacket);
	void SendPacket(const char *IP, const WORD Port, CP2PSerializationBuf *pPacket);

	// 주어진 IP와 Port 번호로 통신을 시도하며,
	// 사용자가 보낸 쌍의 순서로 통신을 시도합니다.
	// IP와 Port 번호가 각각 nullptr, 65535 일 경우 해당 값으로는 시도하지 않습니다.
	// 최대 3개 까지만 IP와 Port를 받아옵니다.
	// 외부에서 해당하는 배열을 넣어주세요.
	void ConnectOtherUser(const char *IP[3], const WORD Port[3]);

private :
	UINT Worker();

	char RecvPost();
	char SendPost();

	void HolePunching(const char* IP, const WORD Port);

	bool P2POptionParsing(const WCHAR* szOptionFileName);

	void CheckOtherUserAlive();
	void UserErase();

	void WriteError(int WindowErr, int UserErr);

	struct st_ConnectTarget
	{
		char										IP[16];
		WORD										Port;
	};

	struct st_OtherUserInfo
	{
		char										NotRecv;
		bool										IsConnectUser;
		UINT64										UserNumber;
		UINT64										BeforeRecvTime;
		UINT64										BeforeSendTime;
		SOCKADDR_IN									SockAddr;
		LPOVERLAPPED								RecvOverlapped;
		LPOVERLAPPED								SendOverlapped;
		std::stack<st_ConnectTarget>				ConnectTargetStack;
	};

	// 몇 번 까지 받는거를 시도할 것인지에 대한 값
	char											m_NotRecvMax;
	// 얼마의 시간 동안 받는거를 시도하고 유저가 나갔다고 판단할 값
	UINT64											m_TimeOutMax;

	UINT64											m_UserNumber;

	SOCKET											m_Socket;

	std::map<LPOVERLAPPED, UINT64>					m_UserFindMap;

	// OtherUserInfo 관리
	std::map<UINT64, st_OtherUserInfo*>				m_OtherUserMap;
	CTLSMemoryPool<st_OtherUserInfo>				*m_pOtherUserMemmoryPool;
};