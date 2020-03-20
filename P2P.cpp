#include "PreCompile.h"
#include "P2P.h"
#include "P2PSerializeBuffer.h"
#include "Log.h"

#pragma comment(lib, "ws2_32")

CP2P::CP2P() : m_NumOfWorkerThread(0), m_NumOfUsingWorkerThread(0),
m_NotRecvMax(0), m_TimeOutMax(0), m_UserID(0), m_Socket(INVALID_SOCKET),
m_pWorkerThreadHandle(nullptr)/*, m_Time(0)*/
{

}

CP2P::~CP2P()
{

}

bool CP2P::Start(const WCHAR* pOptionFileName)
{
	if (!P2POptionParsing(pOptionFileName))
	{
		WriteError(WSAGetLastError(), SERVER_ERR::PARSING_ERR);
		return false;
	}

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

	InitializeSRWLock(&m_OtherUserMapLock);
	InitializeSRWLock(&m_OtherUserFindMapLock);
	m_pOtherUserMemoryPool = new CTLSMemoryPool<st_OtherUserInfo>(1, false);
	m_pSendToTargetMemoryPool = new CTLSMemoryPool<st_SendToTarget>(2, false);

	m_ConnectUserEvnetHandle[en_EVNET_HANDLE_NUMBER::AWAKE_THREAD] = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_RecvEvnetHandle[en_EVNET_HANDLE_NUMBER::AWAKE_THREAD] = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_SendEventHandle[en_EVNET_HANDLE_NUMBER::AWAKE_THREAD] = CreateEvent(NULL, FALSE, FALSE, NULL);

	m_ConnectUserEvnetHandle[en_EVNET_HANDLE_NUMBER::STOP_THREAD] = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_RecvEvnetHandle[en_EVNET_HANDLE_NUMBER::STOP_THREAD] = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_SendEventHandle[en_EVNET_HANDLE_NUMBER::STOP_THREAD] = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_WorkerIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, m_NumOfUsingWorkerThread);
	if (m_WorkerIOCP == NULL)
	{
		WriteError(WSAGetLastError(), SERVER_ERR::WORKERIOCP_NULL_ERR);
		return false;
	}

	m_pWorkerThreadHandle = new HANDLE[m_NumOfWorkerThread];
	for (int i = 0; i < m_NumOfWorkerThread; ++i)
	{
		m_pWorkerThreadHandle[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThreadStartAddress, this, 0, NULL);
		if (m_pWorkerThreadHandle[i] == 0)
		{
			WriteError(WSAGetLastError(), SERVER_ERR::BEGINTHREAD_ERR);
			return false;
		}
	}

	m_SocketObserverSendThreadHandle = (HANDLE)_beginthreadex(NULL, 0, SocketObserverSendThreadStartAddress, this, 0, NULL);
	if (m_SocketObserverSendThreadHandle == 0)
	{
		WriteError(WSAGetLastError(), SERVER_ERR::BEGINTHREAD_ERR);
		return false;
	}

	CreateIoCompletionPort((HANDLE)m_Socket, m_WorkerIOCP,
		(ULONG_PTR)500, NULL);

	return true;
}

void CP2P::Stop()
{
	closesocket(m_Socket);

	PostQueuedCompletionStatus(m_WorkerIOCP, 0, 0, (LPOVERLAPPED)1);

	WaitForMultipleObjects(m_NumOfWorkerThread, m_pWorkerThreadHandle, TRUE, INFINITE);

	for (BYTE i = 0; i < m_NumOfWorkerThread; ++i)
		CloseHandle(m_pWorkerThreadHandle[i]);

	delete[] m_pWorkerThreadHandle;

	WSACleanup();
}

bool CP2P::DisConnect(UINT64 UserID)
{
	// m_OtherUserFindMap 임계영역
	AcquireSRWLockShared(&m_OtherUserFindMapLock);

	auto FindIter = m_OtherUserFindMap.find(UserID);
	if (FindIter == m_OtherUserFindMap.end())
	{
		// TCP에서는 끊겼는데 여기에는 아직 접속하지 않았을 경우
		// 이후 도착한 패킷이 접속 패킷이면 대상이 접속된 대상으로 표현될 수 있음
		// 그 때문에 아직 접속되지 못한 유저가 끊겼을 경우를 대비한 유저맵을 만들어야 하는데
		// 그렇다면 해당 유저를 얼마 동안 맵에 대기시켜야 하는가?


		return true;
	}

	st_OtherUserInfo* pUserInfo = FindIter->second;

	// m_OtherUserFindMap 임계영역
	ReleaseSRWLockShared(&m_OtherUserFindMapLock);

	// m_OtherUserFindMap 임계영역
	AcquireSRWLockExclusive(&m_OtherUserFindMapLock);

	m_OtherUserFindMap.erase(FindIter);

	// m_OtherUserFindMap 임계영역
	ReleaseSRWLockExclusive(&m_OtherUserFindMapLock);

	// m_OtherUserMap 임계영역
	AcquireSRWLockExclusive(&m_OtherUserMapLock);

	m_OtherUserMap.erase(*(st_Sockaddr*)(&pUserInfo->SockAddr));

	// m_OtherUserMap 임계영역
	ReleaseSRWLockExclusive(&m_OtherUserMapLock);

	m_pOtherUserMemoryPool->Free(pUserInfo);

	return true;
}

void CP2P::ConnectOtherUser(const char* IP, const WORD Port)
{
	// 특수한 패킷을 하나 보내어 이 패킷이 접속 패킷을 알림
	CP2PSerializationBuf& HolePuchingPacket = *CP2PSerializationBuf::Alloc();

	HolePuchingPacket.m_pSerializeBuffer[df_PACKET_TYPE_LOCATION] = dfREQ_CONNECT;

	SendPacket(IP, Port, &HolePuchingPacket);
}

void CP2P::SendPacket(UINT64 UserID, CP2PSerializationBuf* pPacket)
{
	// m_OtherUserFindMap 임계영역
	AcquireSRWLockShared(&m_OtherUserFindMapLock);

	auto FindIter = m_OtherUserFindMap.find(UserID);
	if (FindIter == m_OtherUserFindMap.end())
	{
		// 찾고 있는 유저가 없다면 어떤 경우인가?
		return;
	}

	st_OtherUserInfo* FindUser = FindIter->second;

	// m_OtherUserFindMap 임계영역
	ReleaseSRWLockShared(&m_OtherUserFindMapLock);

	st_SendToTarget* pTarget = m_pSendToTargetMemoryPool->Alloc();
	pTarget->Addr.Addr = FindUser->SockAddr;
	pTarget->pBuf = pPacket;

	m_SendQ.Enqueue(pTarget);

	SetEvent(m_SocketObserverSendThreadHandle);
}

void CP2P::SendPacket(const char* IP, const WORD Port, CP2PSerializationBuf* pPacket)
{
	st_SendToTarget* pTarget = m_pSendToTargetMemoryPool->Alloc();

	*(USHORT*)(&pTarget->Addr) = AF_INET;
	*(((USHORT*)(&pTarget->Addr)) + 1) = htons(Port);
	inet_pton(AF_INET, IP, (((UINT*)(&pTarget->Addr)) + 1));

	m_SendQ.Enqueue(pTarget);

	SetEvent(m_SocketObserverSendThreadHandle);
}

UINT CP2P::UserConnectThraed()
{
	DWORD EventRetval;

	while (1)
	{
		EventRetval = WaitForMultipleObjects(en_EVNET_HANDLE_NUMBER::END_OF_EVNET_HANDLE_NUMBER,
			m_ConnectUserEvnetHandle, FALSE, INFINITE) == WAIT_OBJECT_0;
		if (EventRetval == WAIT_OBJECT_0)
		{
			sockaddr Addr;
			m_ConnectUserQueue.Dequeue(&Addr);

			UINT64 ThisUserID = m_UserID;
			++m_UserID;
			st_OtherUserInfo* pNewUserInfo = m_pOtherUserMemoryPool->Alloc();

			//pNewUserInfo->BeforeRecvTime = ServerTime;
			//pNewUserInfo->BeforeSendTime = ServerTime;
			pNewUserInfo->NotRecv = 0;
			pNewUserInfo->UserID = ThisUserID;
			pNewUserInfo->SockAddr = Addr;
			pNewUserInfo->TimeStamp = 0;

			// m_OtherUserMap 임계영역
			AcquireSRWLockExclusive(&m_OtherUserMapLock);

			m_OtherUserMap.insert({ *(st_Sockaddr*)(&Addr), pNewUserInfo });

			// m_OtherUserMap 임계영역
			ReleaseSRWLockExclusive(&m_OtherUserMapLock);

			// m_OtherUserFindMap 임계영역
			AcquireSRWLockExclusive(&m_OtherUserFindMapLock);

			m_OtherUserFindMap.insert({ ThisUserID, pNewUserInfo });

			// m_OtherUserFindMap 임계영역
			ReleaseSRWLockExclusive(&m_OtherUserFindMapLock);

			OtherUserConnected(ThisUserID);
		}
		else if (EventRetval == WAIT_OBJECT_0 + 1)
			break;
		else
		{
			WriteError(WSAGetLastError(), SERVER_ERR::WAIT_OBJECT_ERR);
			g_Dump.Crash();
		}
	}

	return 0;
}

UINT CP2P::SocketObserverRecvThread()
{
	int retval;
	SOCKET& Sock = m_Socket;
	sockaddr Addr;
	int AddrLen = sizeof(Addr);

	st_OtherUserInfo* pRecvUser;

	while (1)
	{
		CP2PSerializationBuf& RecvBuf = *CP2PSerializationBuf::Alloc();

		retval = recvfrom(Sock, RecvBuf.GetBufferPtr(), RecvBuf.GetFreeSize(), 0, &Addr, &AddrLen);
		if (retval == SOCKET_ERROR)
		{
			WriteError(WSAGetLastError(), SERVER_ERR::WAIT_OBJECT_ERR);
			break;
		}

		if (retval >= df_HEADER_SIZE)
		{
			WriteError(0, SERVER_ERR::PACKET_HEADER_SIZE_ERR);
			CP2PSerializationBuf::Free(&RecvBuf);

			// 이미 통신중인 유저라면, 통신을 해제해야함
			// 아래 부분과 상충됨

			continue;
		}

		if (RecvBuf.m_pSerializeBuffer[df_PACKET_TYPE_LOCATION] == dfRES_CONNECT)
		{
			m_ConnectUserQueue.Enqueue(Addr);
			SetEvent(m_ConnectUserEvnetHandle[en_EVNET_HANDLE_NUMBER::AWAKE_THREAD]);
			CP2PSerializationBuf::Free(&RecvBuf);

			continue;
		}

		// OtherUserMap 임계영역
		AcquireSRWLockShared(&m_OtherUserMapLock);

		auto FindIter = m_OtherUserMap.find(*(st_Sockaddr*)(&Addr));
		if (FindIter == m_OtherUserMap.end())
		{
			WriteError(WSAGetLastError(), SERVER_ERR::INCORRECT_SESSION);
			CP2PSerializationBuf::Free(&RecvBuf);

			continue;
		}

		// OtherUserMap 임계영역
		ReleaseSRWLockShared(&m_OtherUserMapLock);

		RecvBuf.MoveWritePos(retval);

		pRecvUser->m_RecvQ.Enqueue(&RecvBuf);
		PostQueuedCompletionStatus(m_WorkerIOCP, retval, (ULONG_PTR)pRecvUser, &m_RecvOverlapped);
	}

	return 0;
}

UINT CP2P::SocketObserverSendThread()
{
	int retval;
	SOCKET& Sock = m_Socket;

	DWORD EventRetval;
	st_SendToTarget* pSendTarget;

	while (1)
	{
		EventRetval = WaitForMultipleObjects(en_EVNET_HANDLE_NUMBER::END_OF_EVNET_HANDLE_NUMBER,
			m_SendEventHandle, FALSE, INFINITE) == WAIT_OBJECT_0;
		if (EventRetval == WAIT_OBJECT_0)
		{
			while (1)
			{
				if (m_SendQ.GetRestSize() == 0)
					break;
				if (!m_SendQ.Dequeue(&pSendTarget))
					break;

				retval = sendto(Sock, pSendTarget->pBuf->GetBufferPtr(), pSendTarget->pBuf->GetAllUseSize()
					, 0, &pSendTarget->Addr.Addr, sizeof(pSendTarget->Addr));

				if (retval == SOCKET_ERROR)
				{
					WriteError(WSAGetLastError(), SERVER_ERR::WAIT_OBJECT_ERR);
					break;
				}
			}
		}
		else if (EventRetval == WAIT_OBJECT_0 + 1)
			break;
		else
		{
			WriteError(WSAGetLastError(), SERVER_ERR::WAIT_OBJECT_ERR);
			g_Dump.Crash();
		}
	}

	return 0;
}

UINT CP2P::WorkerThread()
{
	BOOL GQCSRetval;
	DWORD Transffered;
	st_OtherUserInfo* pUser;
	LPOVERLAPPED lpOverlapped;
	OVERLAPPED RecvOverlapped = &m_RecvOverlapped;

	while (1)
	{
		GQCSRetval = GetQueuedCompletionStatus(m_WorkerIOCP, &Transffered, (PULONG_PTR)&pUser, &lpOverlapped, INFINITE);
		st_OtherUserInfo& IOCompletionUser = *pUser;

		// lpOverlapped 가 NULL 일 경우 오류로 간주함
		if (lpOverlapped == NULL)
		{
			WriteError(WSAGetLastError(), SERVER_ERR::OVERLAPPED_NULL_ERR);

			g_Dump.Crash();
		}

		if (GQCSRetval)
		{
			// 외부 종료코드에 의한 종료
			if (pUser == NULL)
			{
				PostQueuedCompletionStatus(m_WorkerIOCP, 0, 0, (LPOVERLAPPED)1);
				break;
			}

			// recv 파트
			if (lpOverlapped == &RecvOverlapped)
			{
				bool bPacketError = false;
				
				while (IOCompletionUser.m_RecvQ.GetRestSize() > 0)
				{
					CP2PSerializationBuf* pRecvBuf;
					IOCompletionUser.m_RecvQ.Dequeue(&pRecvBuf);

					pRecvBuf->m_iRead = 0;

					BYTE Code;
					WORD PayloadLength;
					*pRecvBuf >> Code >> PayloadLength;

					if (Code != CP2PSerializationBuf::m_byHeaderCode)
					{
						bPacketError = true;
						WriteError(0, SERVER_ERR::HEADER_CODE_ERR);
						CP2PSerializationBuf::Free(pRecvBuf);
						break;
					}
					if (pRecvBuf->GetAllUseSize() < PayloadLength + df_HEADER_SIZE)
					{
						bPacketError = true;
						WriteError(0, SERVER_ERR::PAYLOAD_SIZE_OVER_ERR);
						CP2PSerializationBuf::Free(pRecvBuf);
						break;
					}

					pRecvBuf->MoveWritePos(df_HEADER_SIZE);
					if (!pRecvBuf->Decode())
					{
						bPacketError = true;
						WriteError(0, SERVER_ERR::CHECKSUM_ERR);
						CP2PSerializationBuf::Free(pRecvBuf);
						break;
					}

					char PacketType = pRecvBuf->m_pSerializeBuffer[df_PACKET_TYPE_LOCATION];
					if (PacketType == dfREQ_NORMAL)
					{
						OnRecv(pUser->UserID, pRecvBuf);
						CP2PSerializationBuf::Free(pRecvBuf);
					}
					else if (PacketType == dfRES_NORMAL)
					{
					
					}
					else
					{
						bPacketError = true;
						WriteError(0, SERVER_ERR::PACKET_TYPE_ERR);
						CP2PSerializationBuf::Free(pRecvBuf);
						break;
					}
				}

				if (bPacketError)
					DisConnect(pUser->UserID);
			}
			else
			{
				int err = GetLastError();

				if (err != ERROR_NETNAME_DELETED && err != ERROR_OPERATION_ABORTED)
					GQCSFailed(err);
			}
		}
	}

	return 0;
}

void CP2P::CheckOtherUserAlive()
{

}

bool CP2P::P2POptionParsing(const WCHAR* szOptionFileName)
{
	_wsetlocale(LC_ALL, L"Korean");

	CParser parser;
	WCHAR cBuffer[BUFFER_MAX];

	FILE* fp;
	_wfopen_s(&fp, szOptionFileName, L"rt, ccs=UNICODE");

	int iJumpBOM = ftell(fp);
	fseek(fp, 0, SEEK_END);
	int iFileSize = ftell(fp);
	fseek(fp, iJumpBOM, SEEK_SET);
	int FileSize = (int)fread_s(cBuffer, BUFFER_MAX, sizeof(WCHAR), BUFFER_MAX / 2, fp);
	int iAmend = iFileSize - FileSize; // 개행 문자와 파일 사이즈에 대한 보정값
	fclose(fp);

	cBuffer[iFileSize - iAmend] = '\0';
	WCHAR* pBuff = cBuffer;

	BYTE DebugLevel;

	if (!parser.GetValue_Byte(pBuff, L"SERVER", L"WORKER_THREAD", &m_NumOfWorkerThread))
		return false;
	if (!parser.GetValue_Byte(pBuff, L"SERVER", L"USE_IOCPWORKER", &m_NumOfUsingWorkerThread))
		return false;

	if (!parser.GetValue_Byte(pBuff, L"SERVER", L"PACKET_CODE", &CP2PSerializationBuf::m_byHeaderCode))
		return false;
	if (!parser.GetValue_Byte(pBuff, L"SERVER", L"PACKET_KEY", &CP2PSerializationBuf::m_byXORCode))
		return false;
	if (!parser.GetValue_Byte(pBuff, L"SERVER", L"LOG_LEVEL", &DebugLevel))
		return false;

	SetLogLevel(DebugLevel);

	return true;
}

void CP2P::WriteError(int WindowErr, int UserErr)
{
	st_Error Error;
	Error.GetLastErr = WindowErr;
	Error.ServerErr = UserErr;
	OnError(&Error);
}

UINT CP2P::WorkerThreadStartAddress(LPVOID pP2P)
{
	return ((CP2P*)pP2P)->WorkerThread();
}

UINT CP2P::SocketObserverRecvThreadStartAddress(LPVOID pP2P)
{
	return ((CP2P*)pP2P)->SocketObserverRecvThread();
}

UINT CP2P::SocketObserverSendThreadStartAddress(LPVOID pP2P)
{
	return ((CP2P*)pP2P)->SocketObserverSendThread();
}

UINT CP2P::UserConnectThraedStartAddress(LPVOID pP2P)
{
	return ((CP2P*)pP2P)->UserConnectThraed();
}

