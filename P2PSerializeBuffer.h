#pragma once
#include "LockFreeMemoryPool.h"

#define dfDEFAULTSIZE		512
#define BUFFFER_MAX			10000

// Header �� ũ�⸦ ������
// ���α׷����� ��������
// ��� ũ�⿡ ���� ������ ��
// ���� ���ο��� �ݵ�� 
// WritePtrSetHeader() �� WritePtrSetLast() �� ��������ν�
// ��� ���� �κа� ���̷ε� ������ �κ����� �����͸� �Ű���� ��
// Ȥ�� GetLastWrite() �� ����Ͽ� ����ڰ� �� ������ ���� ������� �Ѱ���
#define df_HEADER_SIZE		6

#define dfNUM_OF_NETBUF_CHUNK		10

#define df_RAND_CODE_LOCATION		 3
#define df_CHECKSUM_CODE_LOCATION	 4
#define df_IS_CONNECT_LOCATION		 5

class CP2P;

class CP2PSerializationBuf
{
private:
	BYTE		m_byError;
	bool		m_bIsEncoded;
	WORD		m_iWrite;
	WORD		m_iRead;
	WORD		m_iSize;
	// WritePtrSetHeader �Լ��� ���� �������� �� ������ �����
	WORD		m_iWriteLast;
	WORD		m_iUserWriteBefore;
	UINT		m_iRefCount;
	char		*m_pSerializeBuffer;
	//static CLockFreeMemoryPool<CSerializationBuf>	*pMemoryPool;

	static CTLSMemoryPool<CP2PSerializationBuf>* pMemoryPool;
	static BYTE												m_byHeaderCode;
	static BYTE												m_byXORCode;

	friend CP2P;
private:
	void Initialize(int BufferSize);

	void SetWriteZero();
	// ���� �������� ������ ������ ������ ����ϰ�
	// ���� �����͸� ����ȭ ���� ���� ó������ �ű�
	void WritePtrSetHeader();
	// ���� �����͸� ������ ����ߴ� ������ ������ �κ����� �̵���Ŵ
	void WritePtrSetLast();

	// ������ ó������ ����ڰ�
	// ���������� �� ���������� ���� ����
	int GetLastWrite();

	char* GetBufferPtr();

	// ���ϴ� ���̸�ŭ �б� ��ġ���� ����
	void RemoveData(int Size);


	// ���� ���� �� ���� ��� �� ��
	static void ChunkFreeForcibly();

	int GetAllUseSize();
public:
	void Encode();
	bool Decode();
	CP2PSerializationBuf();
	~CP2PSerializationBuf();

	void Init();

	void WriteBuffer(char* pData, int Size);
	void ReadBuffer(char* pDest, int Size);
	void PeekBuffer(char* pDest, int Size);

	char* GetReadBufferPtr();
	char* GetWriteBufferPtr();

	// ���� ����ũ�⸦ �Ҵ��Ͽ� ���� �����͸� �ű�
	void Resize(int Size);

	// ���ϴ� ���̸�ŭ �б� ���� ��ġ �̵�
	void MoveWritePos(int Size);
	// ThisPos�� ���� �����͸� �̵���Ŵ
	void MoveWritePosThisPos(int ThisPos);
	// MoveWritePosThisPos �� �̵��ϱ� ���� ���� �����ͷ� �̵���Ŵ
	// �� �Լ��� ������� �ʰ� �� �Լ��� ȣ���� ���
	// ���� �����͸� ���� ó������ �̵���Ŵ
	void MoveWritePosBeforeCallThisPos();


	// --------------- ��ȯ�� ---------------
	// 0 : ����ó�� �Ǿ���
	// 1 : ���۸� �������� �Ͽ�����, �д� ������ ���� ũ�⺸�� ũ�ų� ���� �������� ���� ������
	// 2 : ���۸� ������ �Ͽ�����, ���� ������ ���� ũ�⺸�� ŭ
	// --------------- ��ȯ�� ---------------
	BYTE GetBufferError();

	int GetUseSize();
	int GetFreeSize();

	static CP2PSerializationBuf*		Alloc();
	static void							AddRefCount(CP2PSerializationBuf* AddRefBuf);
	//static void							AddRefCount(CP2PSerializationBuf* AddRefBuf, int AddNumber);
	static void							Free(CP2PSerializationBuf* DeleteBuf);
	static int							GetUsingSerializeBufNodeCount();
	static int							GetUsingSerializeBufChunkCount();
	static int							GetAllocSerializeBufChunkCount();

	CP2PSerializationBuf& operator<<(int Input);
	CP2PSerializationBuf& operator<<(WORD Input);
	CP2PSerializationBuf& operator<<(DWORD Input);
	CP2PSerializationBuf& operator<<(char Input);
	CP2PSerializationBuf& operator<<(BYTE Input);
	CP2PSerializationBuf& operator<<(float Input);
	CP2PSerializationBuf& operator<<(UINT Input);
	CP2PSerializationBuf& operator<<(UINT64 Input);
	CP2PSerializationBuf& operator<<(__int64 Input);

	CP2PSerializationBuf& operator>>(int& Input);
	CP2PSerializationBuf& operator>>(WORD& Input);
	CP2PSerializationBuf& operator>>(DWORD& Input);
	CP2PSerializationBuf& operator>>(char& Input);
	CP2PSerializationBuf& operator>>(BYTE& Input);
	CP2PSerializationBuf& operator>>(float& Input);
	CP2PSerializationBuf& operator>>(UINT& Input);
	CP2PSerializationBuf& operator>>(UINT64& Input);
	CP2PSerializationBuf& operator>>(__int64& Input);
};

