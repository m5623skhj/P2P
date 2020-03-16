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

	// �־��� IP�� Port ��ȣ�� ����� �õ��ϸ�,
	// ����ڰ� ���� ���� ������ ����� �õ��մϴ�.
	// IP�� Port ��ȣ�� ���� nullptr, 65535 �� ��� �ش� �����δ� �õ����� �ʽ��ϴ�.
	// �ִ� 3�� ������ IP�� Port�� �޾ƿɴϴ�.
	// �ܺο��� �ش��ϴ� �迭�� �־��ּ���.
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

	// �� �� ���� �޴°Ÿ� �õ��� �������� ���� ��
	char											m_NotRecvMax;
	// ���� �ð� ���� �޴°Ÿ� �õ��ϰ� ������ �����ٰ� �Ǵ��� ��
	UINT64											m_TimeOutMax;

	UINT64											m_UserNumber;

	SOCKET											m_Socket;

	std::map<LPOVERLAPPED, UINT64>					m_UserFindMap;

	// OtherUserInfo ����
	std::map<UINT64, st_OtherUserInfo*>				m_OtherUserMap;
	CTLSMemoryPool<st_OtherUserInfo>				*m_pOtherUserMemmoryPool;
};