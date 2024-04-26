#include "napi.h"
#include <stdio.h>
#include <intrin.h>


#define htons _byteswap_ushort


HANDLE hIOCP;
NTSTATUS Status;
IO_STATUS_BLOCK ISB;



void WaitForComplete()
{
	DWORD Transferred;
	ULONG_PTR Key;
	OVERLAPPED* pOverlapped;

	GetQueuedCompletionStatus(hIOCP, &Transferred, &Key, &pOverlapped, INFINITE);
}

void DemoConnect()
{
	HANDLE hSocket;
	

	//NxSocket(AF_INET, IPPROTO_TCP, &hSocket);

	CreateIoCompletionPort(hSocket, hIOCP, 0, 0);

	sockaddr_in sa;
	sa.Family = AF_INET;
	sa.Address = 0;
	sa.Port = 0;

	Status = NxBind(hSocket, (sockaddr*) &sa, sizeof(sa), BIND_SHARE_NORMAL);

	sa.Port = htons(80);
	sa.Address = '\x8E\xFA\x42\xCE'; // google.com

	if((Status = NxConnect(hSocket, &ISB, (sockaddr*) &sa, sizeof(sa))) != STATUS_PENDING)
	{
		printf("Failed to initiate connect operation.");
		return;
	}

	WaitForComplete();
	printf("Connect operation complete: 0x%X", ISB.Status);

	NETBUF Nb;
	Nb.Data = (void*) "GET / HTTP/1.1\r\n\r\n\r\n";
	Nb.Length = strlen((const char*) Nb.Data);

	NxSend(hSocket, &ISB, &Nb, 1);

	WaitForComplete();
	printf("\nSend operation complete: 0x%X", ISB.Status);


	static char ReceiveBuffer[0x1000];

	Nb.Data = ReceiveBuffer;
	Nb.Length = sizeof(ReceiveBuffer);

	NxReceive(hSocket, &ISB, &Nb, 1, 0);

	WaitForComplete();

	ReceiveBuffer[sizeof(ReceiveBuffer)-1] = '\0';
	printf("\nReceive operation complete: 0x%X, Received: %u bytes:\r\n%s", ISB.Status, ISB.Information, ReceiveBuffer);
}

void DemoAccept()
{
	HANDLE hSocket;
	HANDLE hAccept;


	//NxSocket(AF_INET, IPPROTO_TCP, &hSocket);
	//NxSocket(AF_INET, IPPROTO_TCP, &hAccept);

	CreateIoCompletionPort(hSocket, hIOCP, 0, 0);

	sockaddr_in sa;
	sa.Family = AF_INET;
	sa.Address = 0;
	sa.Port = htons(8888);

	NxBind(hSocket, (sockaddr*) &sa, sizeof(sa), BIND_SHARE_EXCLUSIVE);
	NxListen(hSocket, 0);

	struct
	{
		alignas(MAX_NETWORK_ADDRESS_SZ) sockaddr_in LocalAddr;
		alignas(MAX_NETWORK_ADDRESS_SZ) sockaddr_in RemoteAddr;
	} Buffer;

	// Unlike 'AcceptEx`, `NxAccept` takes a singular buffer that serves as both the buffer for auxillary (address-) data and
	// also optionally an initial receive I/O buffer, partitioned as follows:
	//
	// +----------------+----------------+----------------
	// | Local Address  | Remote Address | Receive I/O ...
	// +----------------+----------------+----------------
	//  
	// The individual local/remote address buffers are a fixed size of `MAX_NETWORK_ADDRESS_SZ` and are always returned. Any
	// additional buffer space provided implies the desire for delayed acceptance behavior, where-in completion will only occur
	// once atleast 1 byte of application-layer data is received. The size of the receive buffer is simply calculated as the
	// `BufferLength` minus `2*MAX_NETWORK_ADDRESS_SZ`, with zero meaning to disable delayed acceptance.
	//
	NxAccept(hSocket, &ISB, hAccept, &Buffer, sizeof(Buffer));

	WaitForComplete();

	printf("\nAccept operation complete: 0x%X", ISB.Status);
}


void main()
{
	// Currently NAPI only exposes support for IOCP-based IRP completion notification, as the expected use-case is 
	// for server software.
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);


	//DemoAccept();
	//DemoConnect();

	auto r = NxEnableRegisteredIo();


	void* Region = VirtualAlloc(NULL, 0x2000, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	ULONG RegionId;
	auto res = NxRegisterIoRegion(Region, 0x2000, &RegionId);


	RIO_NOTIFY Notify;
	Notify.Type = RIO_NOTIFY_EVENT;
	Notify.Event.EventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
	Notify.Event.NotifyReset = FALSE;

	void* Ring = VirtualAlloc(NULL, sizeof(RIO_RING) + 1024 * sizeof(RIO_COMPLETION_ENTRY), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	ULONG RingId = 0xDDEE;

	auto Status = NxRegisterCompletionRing(Ring, 1024, &Notify, &RingId);
}