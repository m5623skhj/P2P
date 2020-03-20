#pragma once
#include "P2P.h"

class P2PTester : public CP2P
{
public :
	P2PTester();
	~P2PTester();

	void StartP2P();
	void StopP2P();

	void ShootTarget();

private :
	virtual void OtherUserConnected(UINT64 UserID);
	// 다른 유저가 나감
	virtual void OtherUserDisConnected(UINT64 UserID);

	// 패킷 수신 완료 후 호출됨
	virtual void OnRecv(UINT64 UserID, CP2PSerializationBuf* pPacket);
	// 패킷 송신 완료 후 호출됨
	virtual void OnSend(UINT64 UserID, int SendSize);

	// 워커스레드 GQCS 바로 하단에서 호출됨
	virtual void OnWorkerThreadBegin();
	// 워커스레드 1루프 종료 후 호출됨
	virtual void OnWorkerThreadEnd();

	// 해당 IP와 Port 번호로의 통신이 실패한것으로 판단되면 호출됨
	virtual void OnConnectOtherUserFail(UINT64 UserID);

	// 사용자 에러 처리 함수
	virtual void OnError(st_Error* OutError);

	// GQCS 함수 호출이 실패하면 호출됨
	virtual void GQCSFailed(int LastError);
};