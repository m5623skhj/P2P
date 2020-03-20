#pragma once
#include "PreCompile.h"
#include "Ringbuffer.h"
#include "LockFreeQueue.h"
#include "ServerCommon.h"
#include <map>
#include <functional>
#include "Queue.h"

// �ش� ������ �۽��߿� �ִ��� �ƴ���
#define NONSENDING	0
#define SENDING		1

// Recv / Send Post ��ȯ ��
#define POST_RETVAL_ERR_SESSION_DELETED		0
#define POST_RETVAL_ERR						1
#define POST_RETVAL_COMPLETE				2

#define ONE_SEND_WSABUF_MAX					200

class CP2PSerializationBuf;

class CP2P
{
public :
	CP2P();
	~CP2P();

	UINT									RecvTPS;
	UINT									SendTPS;

protected :
	bool Start(const WCHAR *pOptionFileName);
	void Stop();

	bool DisConnect(UINT64 UserID);

	// �ٸ� ������ ����
	virtual void OtherUserConnected(UINT64 UserID) = 0;
	// �ٸ� ������ ����
	virtual void OtherUserDisConnected(UINT64 UserID) = 0;

	// ��Ŷ ���� �Ϸ� �� ȣ���
	virtual void OnRecv(UINT64 UserID, CP2PSerializationBuf *pPacket) = 0;
	// ��Ŷ �۽� �Ϸ� �� ȣ���
	virtual void OnSend(UINT64 UserID, int SendSize) = 0;

	// ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ���
	virtual void OnWorkerThreadBegin() = 0;
	// ��Ŀ������ 1���� ���� �� ȣ���
	virtual void OnWorkerThreadEnd() = 0;

	void SendPacket(UINT64 UserID, CP2PSerializationBuf*pPacket);

	// �ش� IP�� Port ��ȣ���� ����� �����Ѱ����� �ǴܵǸ� ȣ���
	virtual void OnConnectOtherUserFail(UINT64 UserID) = 0;

	// ����ڰ� ���� ������ ����� �õ��մϴ�.
	void ConnectOtherUser(const char* IP, const WORD Port);

	// ����� ���� ó�� �Լ�
	virtual void OnError(st_Error* OutError) = 0;

	// GQCS �Լ� ȣ���� �����ϸ� ȣ���
	virtual void GQCSFailed(int LastError) = 0;

private :
	struct st_Sockaddr
	{
		sockaddr									Addr;

		bool operator < (const st_Sockaddr& rhs) const
		{
			UINT64 Highlhs, Highrhs;
			Highlhs = *(UINT64*)(&Addr);
			Highrhs = *(UINT64*)(&rhs);

			if (Highlhs < Highrhs)
				return true;
			else if (Highlhs > Highrhs)
				return false;
			
			UINT64 Lowlhs, Lowrhs;
			Lowlhs = *(UINT64*)((char*)&Addr + 8);
			Lowrhs = *(UINT64*)((char*)&rhs + 8);

			if (Lowlhs < Lowrhs)
				return true;
			return false;
		}
	};

	struct st_SendToTarget
	{
		st_Sockaddr									Addr;
		CP2PSerializationBuf*						pBuf;
	};

	struct st_OtherUserInfo
	{
		char										NotRecv;
		WORD										TimeStamp;
		UINT64										UserID;
		UINT64										BeforeRecvTime;
		UINT64										BeforeSendTime;
		sockaddr									SockAddr;
		CLockFreeQueue<CP2PSerializationBuf*>		m_RecvQ;
	};

	enum en_EVNET_HANDLE_NUMBER
	{
		AWAKE_THREAD = 0,
		STOP_THREAD = 1,
		END_OF_EVNET_HANDLE_NUMBER,
	};

	static UINT __stdcall WorkerThreadStartAddress(LPVOID CP2PPointer);
	UINT WorkerThread();

	static UINT __stdcall SocketObserverRecvThreadStartAddress(LPVOID CP2PPointer);
	UINT SocketObserverRecvThread();

	static UINT __stdcall SocketObserverSendThreadStartAddress(LPVOID CP2PPointer);
	UINT SocketObserverSendThread();

	static UINT __stdcall UserConnectThraedStartAddress(LPVOID CP2PPointer);
	UINT UserConnectThraed();

	void SendPacket(const char* IP, const WORD Port, CP2PSerializationBuf* pPacket);

	bool P2POptionParsing(const WCHAR* szOptionFileName);

	void CheckOtherUserAlive();

	void WriteError(int WindowErr, int UserErr);

	BYTE											m_NumOfWorkerThread;
	BYTE											m_NumOfUsingWorkerThread;
	// �� �� ���� �޴°Ÿ� �õ��� �������� ���� ��
	char											m_NotRecvMax;
	// ���� �ð� ���� �޴°Ÿ� �õ��ϰ� ������ �����ٰ� �Ǵ��� ��
	UINT64											m_TimeOutMax;

	UINT64											m_UserID;

	//UINT64											m_Time;

	SOCKET											m_Socket;

	// UserID�� ������ ������ ã������ ��
	std::map<UINT64, st_OtherUserInfo*>				m_OtherUserFindMap;
	// m_OtherUserFindMap ���� ��
	SRWLOCK											m_OtherUserFindMapLock;

	// OtherUserInfo ����
	std::map<st_Sockaddr, st_OtherUserInfo*>		m_OtherUserMap;
	// m_OtherUserMap ���� ��
	SRWLOCK											m_OtherUserMapLock;

	// �ܺο����� �������ٰ� �Ǵ��Ͽ�����, ���� ����� ���� ������ �ȵ� �������� ��
	// �󸶵��� �� ������ ���ϰ� �������� ����� �� ��.
	//std::set<UINT64>

	Queue<sockaddr>									m_ConnectUserQueue;
	HANDLE											m_ConnectUserEvnetHandle[2];

	CTLSMemoryPool<st_OtherUserInfo>				*m_pOtherUserMemoryPool;

	CTLSMemoryPool<st_SendToTarget>					*m_pSendToTargetMemoryPool;

	HANDLE											m_SocketObserverSendThreadHandle;

	HANDLE											m_RecvEvnetHandle[2];
	HANDLE											m_SendEventHandle[2];

	HANDLE											*m_pWorkerThreadHandle;
	HANDLE											m_WorkerIOCP;

	OVERLAPPED										m_RecvOverlapped;

	sockaddr										m_addr;
	CRingbuffer										m_RingBuffer;

	CLockFreeQueue<st_SendToTarget*>				m_SendQ;
};