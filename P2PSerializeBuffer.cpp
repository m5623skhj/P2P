#include "PreCompile.h"
#include "P2PSerializeBuffer.h"

CTLSMemoryPool<CP2PSerializationBuf>* CP2PSerializationBuf::pMemoryPool = new CTLSMemoryPool<CP2PSerializationBuf>(dfNUM_OF_NETBUF_CHUNK, false);
extern CParser g_Paser;

BYTE CP2PSerializationBuf::m_byHeaderCode = 0;
BYTE CP2PSerializationBuf::m_byXORCode = 0;

struct st_Exception
{
	WCHAR ErrorCode[32];
	WCHAR szErrorComment[1024];
};

CP2PSerializationBuf::CP2PSerializationBuf() :
	m_byError(0), m_bIsEncoded(FALSE), m_iWrite(df_HEADER_SIZE), m_iRead(df_HEADER_SIZE), m_iWriteLast(0),
	m_iUserWriteBefore(df_HEADER_SIZE), m_iRefCount(1)
{
	Initialize(dfDEFAULTSIZE);
}

CP2PSerializationBuf::~CP2PSerializationBuf()
{
	delete[] m_pSerializeBuffer;
}

void CP2PSerializationBuf::Initialize(int BufferSize)
{
	m_iSize = BufferSize;
	m_pSerializeBuffer = new char[BufferSize];
}

void CP2PSerializationBuf::Init()
{
	m_byError = 0;
	m_iWrite = df_HEADER_SIZE;
	m_iRead = df_HEADER_SIZE;
	m_iWriteLast = 0;
	m_iUserWriteBefore = df_HEADER_SIZE;
	m_iRefCount = 1;
}

void CP2PSerializationBuf::Resize(int Size)
{
	if (BUFFFER_MAX < Size)
	{
		m_byError = 3;
		st_Exception e;
		wcscpy_s(e.ErrorCode, L"ErrorCode : 3");
		wsprintf(e.szErrorComment,
			L"%s Line %d\n\n버퍼의 크기를 재할당하려고 하였으나, 버퍼 최대 할당가능공간(10000byte) 보다 더 큰 값을 재할당하려고 하였습니다.\nWrite = %d Read = %d BufferSize = %d InputSize = %d\n\n프로그램을 종료합니다"
			, TEXT(__FILE__), __LINE__, m_iWrite, m_iRead, m_iSize, Size);
		throw e;
	}

	char* pBuffer = new char[Size];
	memcpy_s(pBuffer, m_iWrite, m_pSerializeBuffer, m_iWrite);
	delete[] m_pSerializeBuffer;

	m_pSerializeBuffer = pBuffer;
	m_iSize = Size;
}

void CP2PSerializationBuf::WriteBuffer(char* pData, int Size)
{
	if (BUFFFER_MAX < m_iWrite + Size)
	{
		m_byError = 2;
		st_Exception e;
		wcscpy_s(e.ErrorCode, L"ErrorCode : 2");
		wsprintf(e.szErrorComment,
			L"%s Line %d\n\n버퍼에 쓰려고 하였으나, 버퍼 공간보다 더 큰 값이 들어왔습니다.\nWrite = %d Read = %d BufferSize = %d InputSize = %d\n\n프로그램을 종료합니다"
			, TEXT(__FILE__), __LINE__, m_iWrite, m_iRead, m_iSize, Size);
		throw e;
	}

	switch (Size)
	{
	case 1:
		*((char*)(&m_pSerializeBuffer[m_iWrite])) = *(char*)pData;
		break;
	case 2:
		*((short*)(&m_pSerializeBuffer[m_iWrite])) = *(short*)pData;
		break;
	case 4:
		*((int*)(&m_pSerializeBuffer[m_iWrite])) = *(int*)pData;
		break;
	case 8:
		*((long long*)(&m_pSerializeBuffer[m_iWrite])) = *(long long*)pData;
		break;
	default:
		memcpy_s(&m_pSerializeBuffer[m_iWrite], Size, pData, Size);
		break;
	}
	m_iWrite += Size;
}

void CP2PSerializationBuf::ReadBuffer(char* pDest, int Size)
{
	if (m_iSize < m_iRead + Size || m_iWrite < m_iRead + Size)
	{
		m_byError = 1;
		st_Exception e;
		wcscpy_s(e.ErrorCode, L"ErrorCode : 1");
		wsprintf(e.szErrorComment,
			L"%s Line %d\n\n버퍼를 읽으려고 하였으나, 읽으려고 했던 공간이 버퍼 크기보다 크거나 아직 쓰여있지 않은 공간입니다.\nWrite = %d Read = %d BufferSize = %d InputSize = %d\n\n프로그램을 종료합니다"
			, TEXT(__FILE__), __LINE__, m_iWrite, m_iRead, m_iSize, Size);
		throw e;
	}

	switch (Size)
	{
	case 1:
		*((char*)(pDest)) = *(char*)&m_pSerializeBuffer[m_iRead];
		break;
	case 2:
		*((short*)(pDest)) = *(short*)&m_pSerializeBuffer[m_iRead];
		break;
	case 4:
		*((int*)(pDest)) = *(int*)&m_pSerializeBuffer[m_iRead];
		break;
	case 8:
		*((long long*)(pDest)) = *(long long*)&m_pSerializeBuffer[m_iRead];
		break;
	default:
		memcpy_s(pDest, Size, &m_pSerializeBuffer[m_iRead], Size);
		break;
	}
	m_iRead += Size;
}

void CP2PSerializationBuf::PeekBuffer(char* pDest, int Size)
{
	memcpy_s(pDest, Size, &m_pSerializeBuffer[m_iRead], Size);
}

void CP2PSerializationBuf::RemoveData(int Size)
{
	if (m_iSize < m_iRead + Size || m_iWrite < m_iRead + Size)
	{
		m_byError = 1;
		st_Exception e;
		wcscpy_s(e.ErrorCode, L"ErrorCode : 1");
		wsprintf(e.szErrorComment,
			L"%s Line %d\n\n버퍼를 읽으려고 하였으나, 읽으려고 했던 공간이 버퍼 크기보다 크거나 아직 쓰여있지 않은 공간입니다.\nWrite = %d Read = %d BufferSize = %d InputSize = %d\n\n프로그램을 종료합니다"
			, TEXT(__FILE__), __LINE__, m_iWrite, m_iRead, m_iSize, Size);
		throw e;
	}
	m_iRead += Size;
}

void CP2PSerializationBuf::MoveWritePos(int Size)
{
	if (m_iSize < m_iWrite + Size + df_HEADER_SIZE)
	{
		m_byError = 2;
		st_Exception e;
		wcscpy_s(e.ErrorCode, L"ErrorCode : 2");
		wsprintf(e.szErrorComment,
			L"%s Line %d\n\n버퍼에 쓰려고 하였으나, 버퍼 공간보다 더 큰 값이 들어왔습니다.\nWrite = %d Read = %d BufferSize = %d InputSize = %d\n\n프로그램을 종료합니다"
			, TEXT(__FILE__), __LINE__, m_iWrite, m_iRead, m_iSize, Size);
		throw e;
	}
	m_iWrite += Size;
}

void CP2PSerializationBuf::MoveWritePosThisPos(int ThisPos)
{
	m_iUserWriteBefore = m_iWrite;
	m_iWrite = df_HEADER_SIZE + ThisPos;
}

void CP2PSerializationBuf::MoveWritePosBeforeCallThisPos()
{
	m_iWrite = m_iUserWriteBefore;
}

BYTE CP2PSerializationBuf::GetBufferError()
{
	return m_byError;
}

char* CP2PSerializationBuf::GetBufferPtr()
{
	return m_pSerializeBuffer;
}

char* CP2PSerializationBuf::GetReadBufferPtr()
{
	return &m_pSerializeBuffer[m_iRead];
}

char* CP2PSerializationBuf::GetWriteBufferPtr()
{
	return &m_pSerializeBuffer[m_iWrite];
}

int CP2PSerializationBuf::GetUseSize()
{
	return m_iWrite - m_iRead;// - df_HEADER_SIZE;
}

int CP2PSerializationBuf::GetFreeSize()
{
	return m_iSize - m_iWrite;
}

int CP2PSerializationBuf::GetAllUseSize()
{
	return m_iWriteLast - m_iRead;
}

void CP2PSerializationBuf::SetWriteZero()
{
	m_iWrite = 0;
}

int CP2PSerializationBuf::GetLastWrite()
{
	return m_iWriteLast;
}

void CP2PSerializationBuf::WritePtrSetHeader()
{
	m_iWriteLast = m_iWrite;
	m_iWrite = 0;
}

void CP2PSerializationBuf::WritePtrSetLast()
{
	m_iWrite = m_iWriteLast;
}

// 헤더 순서
// Code(1) Length(2) Random XOR Code(1) CheckSum(1) IsConnectPacket(1)
void CP2PSerializationBuf::Encode()
{
	// 헤더 코드 삽입
	int NowWrite = 0;
	char* (&pThisBuffer) = m_pSerializeBuffer;
	pThisBuffer[NowWrite] = m_byHeaderCode;
	++NowWrite;

	// 페이로드 크기 삽입
	int WirteLast = m_iWriteLast;
	//int WirteLast = m_iWriteLast;
	int Read = m_iRead;
	short PayloadLength = WirteLast - df_HEADER_SIZE;
	*((short*)(&pThisBuffer[NowWrite])) = *(short*)&PayloadLength;
	NowWrite += 2;

	// 난수 XOR 코드 생성
	BYTE RandCode = (BYTE)(rand() & 255);
	BYTE XORCode = m_byXORCode;

	// 체크섬 생성 및 페이로드와 체크섬 암호화
	WORD PayloadSum = 0;
	BYTE BeforeRandomXORByte = 0;
	BYTE BeforeFixedXORByte = 0;
	WORD NumOfRootine = 1;

	for (int BufIdx = df_HEADER_SIZE; BufIdx < WirteLast; ++BufIdx)
		PayloadSum += pThisBuffer[BufIdx];

	pThisBuffer[df_CHECKSUM_CODE_LOCATION] = (BYTE)(PayloadSum & 255);

	// 체크섬 부터 암호화를 할 것이기 때문에
	// df_CHECKSUM_CODE_LOCATION 부터 반복문을 실행함
	for (int BufIdx = df_CHECKSUM_CODE_LOCATION; BufIdx < WirteLast; ++BufIdx)
	{
		BeforeRandomXORByte = pThisBuffer[BufIdx] ^ (BeforeRandomXORByte + RandCode + NumOfRootine);
		BeforeFixedXORByte = BeforeRandomXORByte ^ (BeforeFixedXORByte + XORCode + NumOfRootine);
		pThisBuffer[BufIdx] = BeforeFixedXORByte;
		++NumOfRootine;
	}

	// 암호화 된 랜덤코드 삽입
	pThisBuffer[df_RAND_CODE_LOCATION] = RandCode;

	m_iWrite = df_HEADER_SIZE;
	m_bIsEncoded = TRUE;
}

// 헤더 순서
// Code(1) Length(2) Random XOR Code(1) CheckSum(1) IsConnectPacket(1)
bool CP2PSerializationBuf::Decode()
{
	char* (&pThisBuffer) = m_pSerializeBuffer;
	BYTE RandCode = pThisBuffer[df_RAND_CODE_LOCATION];

	// 체크섬 생성 및 페이로드와 체크섬 복호화
	WORD PayloadSum = 0;
	BYTE BeforeRandomXORByte = 0;
	BYTE BeforeEncodedValue = 0;
	WORD NumOfRootine = 1;
	BYTE XORCode = m_byXORCode;

	BYTE SaveRandom;
	BYTE SaveEncoded;

	SaveEncoded = pThisBuffer[df_CHECKSUM_CODE_LOCATION];
	SaveRandom = pThisBuffer[df_CHECKSUM_CODE_LOCATION] ^ (XORCode + NumOfRootine);
	pThisBuffer[df_CHECKSUM_CODE_LOCATION] = SaveRandom ^ (RandCode + NumOfRootine);

	++NumOfRootine;
	BeforeEncodedValue = SaveEncoded;
	BeforeRandomXORByte = SaveRandom;

	// 체크섬은 이미 복호화 되었으므로
	// df_HEADER_SIZE 부터 반복문을 실행함
	for (int BufIdx = df_HEADER_SIZE; BufIdx < m_iWrite; ++BufIdx)
	{
		SaveEncoded = pThisBuffer[BufIdx];
		SaveRandom = pThisBuffer[BufIdx] ^ (BeforeEncodedValue + XORCode + NumOfRootine);
		pThisBuffer[BufIdx] = SaveRandom ^ (BeforeRandomXORByte + RandCode + NumOfRootine);

		++NumOfRootine;
		PayloadSum += pThisBuffer[BufIdx];
		BeforeEncodedValue = SaveEncoded;
		BeforeRandomXORByte = SaveRandom;
	}

	if (((BYTE)pThisBuffer[df_CHECKSUM_CODE_LOCATION]) != (PayloadSum & 255))
		return false;

	m_iRead = df_HEADER_SIZE;
	return true;
}

/*
// 헤더 순서
// Code(1) Length(2) Random XOR Code(1) CheckSum(1)
void CP2PSerializationBuf::Encode()
{
	// 헤더 코드 삽입
	int NowWrite = 0;
	char *(&pThisBuffer) = m_pSerializeBuffer;
	pThisBuffer[NowWrite] = m_byHeaderCode;
	++NowWrite;

	// 페이로드 크기 삽입
	int WirteLast = m_iWriteLast;
	int Read = m_iRead;
	short PayloadLength = WirteLast - df_HEADER_SIZE;
	*((short*)(&pThisBuffer[NowWrite])) = *(short*)&PayloadLength;
	NowWrite += 2;

	// 난수 XOR 코드 생성
	BYTE RandCode = (BYTE)((rand() & 255) ^ m_byXORCode);

	// 체크섬 생성 및 페이로드와 체크섬 암호화
	WORD PayloadSum = 0;
	for (int BufIdx = df_HEADER_SIZE; BufIdx < WirteLast; ++BufIdx)
	{
		PayloadSum += pThisBuffer[BufIdx];
		pThisBuffer[BufIdx] ^= RandCode;
	}
	BYTE CheckSum = (BYTE)(PayloadSum & 255) ^ RandCode;

	// 암호화 된 랜덤코드와 체크섬 삽입
	pThisBuffer[df_RAND_CODE_LOCATION] = RandCode;
	pThisBuffer[df_CHECKSUM_CODE_LOCATION] = CheckSum;

	m_iWrite = df_HEADER_SIZE;
	m_bIsEncoded = TRUE;
}

// 헤더 순서
// Code(1) Length(2) Random XOR Code(1) CheckSum(1)
bool CP2PSerializationBuf::Decode()
{
	char *(&pThisBuffer) = m_pSerializeBuffer;
	WORD PayloadSum = 0;
	int Write = m_iWrite;
	BYTE RandCode = pThisBuffer[df_RAND_CODE_LOCATION];

	for (int BufIdx = df_HEADER_SIZE; BufIdx < Write; ++BufIdx)
	{
		pThisBuffer[BufIdx] ^= RandCode;
		PayloadSum += pThisBuffer[BufIdx];
	}

	if (((BYTE)pThisBuffer[df_CHECKSUM_CODE_LOCATION] ^ RandCode) != (PayloadSum & 255))
		return false;

	m_iRead = df_HEADER_SIZE;
	return true;
}
*/

//////////////////////////////////////////////////////////////////
// static
//////////////////////////////////////////////////////////////////

CP2PSerializationBuf* CP2PSerializationBuf::Alloc()
{
	CP2PSerializationBuf* pBuf = pMemoryPool->Alloc();
	return pBuf;
}

void CP2PSerializationBuf::AddRefCount(CP2PSerializationBuf* AddRefBuf)
{
	InterlockedIncrement(&AddRefBuf->m_iRefCount);
}

//void CP2PSerializationBuf::AddRefCount(CP2PSerializationBuf* AddRefBuf, int AddNumber)
//{
//	InterlockedAdd((LONG*)&AddRefBuf->m_iRefCount, AddNumber);
//}

void CP2PSerializationBuf::Free(CP2PSerializationBuf* DeleteBuf)
{
	if (InterlockedDecrement(&DeleteBuf->m_iRefCount) == 0)
	{
		DeleteBuf->m_byError = 0;
		DeleteBuf->m_bIsEncoded = FALSE;
		DeleteBuf->m_iWrite = df_HEADER_SIZE;
		DeleteBuf->m_iRead = df_HEADER_SIZE;
		DeleteBuf->m_iWriteLast = 0;
		DeleteBuf->m_iUserWriteBefore = df_HEADER_SIZE;
		DeleteBuf->m_iRefCount = 1;
		CP2PSerializationBuf::pMemoryPool->Free(DeleteBuf);
	}
}

void CP2PSerializationBuf::ChunkFreeForcibly()
{
	CP2PSerializationBuf::pMemoryPool->ChunkFreeForcibly();
}

int CP2PSerializationBuf::GetUsingSerializeBufNodeCount()
{
	return CP2PSerializationBuf::pMemoryPool->GetUseNodeCount();
}

int CP2PSerializationBuf::GetUsingSerializeBufChunkCount()
{
	return CP2PSerializationBuf::pMemoryPool->GetUseChunkCount();
}

int CP2PSerializationBuf::GetAllocSerializeBufChunkCount()
{
	return CP2PSerializationBuf::pMemoryPool->GetAllocChunkCount();
}

//////////////////////////////////////////////////////////////////
// Operator <<
//////////////////////////////////////////////////////////////////

CP2PSerializationBuf& CP2PSerializationBuf::operator<<(int Input)
{
	WriteBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator<<(WORD Input)
{
	WriteBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator<<(DWORD Input)
{
	WriteBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator<<(UINT Input)
{
	WriteBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator<<(UINT64 Input)
{
	WriteBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator<<(char Input)
{
	WriteBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator<<(BYTE Input)
{
	WriteBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator<<(float Input)
{
	WriteBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator<<(__int64 Input)
{
	WriteBuffer((char*)&Input, sizeof(Input));
	return *this;
}

//////////////////////////////////////////////////////////////////
// Operator >>
//////////////////////////////////////////////////////////////////

CP2PSerializationBuf& CP2PSerializationBuf::operator>>(int& Input)
{
	ReadBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator>>(WORD& Input)
{
	ReadBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator>>(DWORD& Input)
{
	ReadBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator>>(char& Input)
{
	ReadBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator>>(BYTE& Input)
{
	ReadBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator>>(float& Input)
{
	ReadBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator>>(UINT& Input)
{
	ReadBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator>>(UINT64& Input)
{
	ReadBuffer((char*)&Input, sizeof(Input));
	return *this;
}

CP2PSerializationBuf& CP2PSerializationBuf::operator>>(__int64& Input)
{
	ReadBuffer((char*)&Input, sizeof(Input));
	return *this;
}