#pragma once
#include "PreCompile.h"
#include "Ringbuffer.h"
#include "LockFreeQueue.h"
#include "ServerCommon.h"
#include <map>
#include <functional>
#include "Queue.h"

// 해당 소켓이 송신중에 있는지 아닌지
#define NONSENDING	0
#define SENDING		1

// Recv / Send Post 반환 값
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

	// 다른 유저가 들어옴
	virtual void OtherUserConnected(UINT64 UserID) = 0;
	// 다른 유저가 나감
	virtual void OtherUserDisConnected(UINT64 UserID) = 0;

	// 패킷 수신 완료 후 호출됨
	virtual void OnRecv(UINT64 UserID, CP2PSerializationBuf *pPacket) = 0;
	// 패킷 송신 완료 후 호출됨
	virtual void OnSend(UINT64 UserID, int SendSize) = 0;

	// 워커스레드 GQCS 바로 하단에서 호출됨
	virtual void OnWorkerThreadBegin() = 0;
	// 워커스레드 1루프 종료 후 호출됨
	virtual void OnWorkerThreadEnd() = 0;

	void SendPacket(UINT64 UserID, CP2PSerializationBuf*pPacket);

	// 해당 IP와 Port 번호로의 통신이 실패한것으로 판단되면 호출됨
	virtual void OnConnectOtherUserFail(UINT64 UserID) = 0;

	// 사용자가 보낸 쌍으로 통신을 시도합니다.
	void ConnectOtherUser(const char* IP, const WORD Port);

	// 사용자 에러 처리 함수
	virtual void OnError(st_Error* OutError) = 0;

	// GQCS 함수 호출이 실패하면 호출됨
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
	// 몇 번 까지 받는거를 시도할 것인지에 대한 값
	char											m_NotRecvMax;
	// 얼마의 시간 동안 받는거를 시도하고 유저가 나갔다고 판단할 값
	UINT64											m_TimeOutMax;

	UINT64											m_UserID;

	//UINT64											m_Time;

	SOCKET											m_Socket;

	// UserID로 유저의 정보를 찾기위한 맵
	std::map<UINT64, st_OtherUserInfo*>				m_OtherUserFindMap;
	// m_OtherUserFindMap 전용 락
	SRWLOCK											m_OtherUserFindMapLock;

	// OtherUserInfo 관리
	std::map<st_Sockaddr, st_OtherUserInfo*>		m_OtherUserMap;
	// m_OtherUserMap 전용 락
	SRWLOCK											m_OtherUserMapLock;

	// 외부에서는 끊어졌다고 판단하였으나, 내부 통신은 아직 연결이 안된 유저들의 맵
	// 얼마동안 이 유저를 지니고 있을지는 고민해 볼 것.
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