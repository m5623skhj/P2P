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
	// �ٸ� ������ ����
	virtual void OtherUserDisConnected(UINT64 UserID);

	// ��Ŷ ���� �Ϸ� �� ȣ���
	virtual void OnRecv(UINT64 UserID, CP2PSerializationBuf* pPacket);
	// ��Ŷ �۽� �Ϸ� �� ȣ���
	virtual void OnSend(UINT64 UserID, int SendSize);

	// ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ���
	virtual void OnWorkerThreadBegin();
	// ��Ŀ������ 1���� ���� �� ȣ���
	virtual void OnWorkerThreadEnd();

	// �ش� IP�� Port ��ȣ���� ����� �����Ѱ����� �ǴܵǸ� ȣ���
	virtual void OnConnectOtherUserFail(UINT64 UserID);

	// ����� ���� ó�� �Լ�
	virtual void OnError(st_Error* OutError);

	// GQCS �Լ� ȣ���� �����ϸ� ȣ���
	virtual void GQCSFailed(int LastError);
};