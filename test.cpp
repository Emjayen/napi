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


	NxSocket(AF_INET, SOCK_TCP, 0, &hSocket);

	CreateIoCompletionPort(hSocket, hIOCP, 0, 0);

	sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = 0;
	sa.sin_port = 0;

	NxBind(hSocket, (sockaddr*) &sa, sizeof(sa), BIND_SHARE_NORMAL);

	union
	{
		BYTE a[4];
		ULONG v;
	} dst = { 45, 79, 112, 203 };

	sa.sin_port = htons(4242);
	sa.sin_addr.s_addr = dst.v;

	if((Status = NxConnect(hSocket, &ISB, (sockaddr*) &sa, sizeof(sa))) != STATUS_PENDING)
	{
		printf("Failed to initiate connect operation.");
		return;
	}

	WaitForComplete();
	printf("Connect operation complete: 0x%X", ISB.Status);

	NETBUF Nb;
	Nb.Data = (void*) "Echo\r\n";
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


	NxSocket(AF_INET, SOCK_TCP, 0, &hSocket);
	NxSocket(AF_INET, SOCK_TCP, 0, &hAccept);

	CreateIoCompletionPort(hSocket, hIOCP, 0, 0);

	sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = 0;
	sa.sin_port = htons(8888);

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


void RioDemo()
{
	NTSTATUS Status;
	HANDLE Socket;
	sockaddr_in sa;
	ULONG IoRegionId;
	ULONG CompletionRingId;
	HANDLE CompletionQueueEvent;
	BYTE* IoRegion;
	RIO_COMPLETION_QUEUE* CompletionQueue;
	RIO_REQUEST_QUEUE* TxRequestQueue;
	RIO_REQUEST_QUEUE* RxRequestQueue;

	// Required global initialization for RIO usage.
	NxEnableRegisteredIo();

	// Create RIO-enabled socket.
	NxSocket(AF_INET, IPPROTO_UDP, SOCK_FLAG_RIO, &Socket);

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = 0;
	sa.sin_port = htons(7755);

	NxBind(Socket, (sockaddr*) &sa, sizeof(sa), BIND_SHARE_NORMAL);

	// Queue (ring) sizes.
	const auto IO_REGION_SZ = 0x10000;
	const auto TX_RING_SZ = 1025;
	const auto RX_RING_SZ = 1025;
	const auto CQ_RING_SZ = TX_RING_SZ + RX_RING_SZ;

	// Register region of memory from which I/O buffers will be allocated.
	IoRegion = (BYTE*) VirtualAlloc(NULL, 0x2000, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	Status = NxRegisterIoRegion(IoRegion, 0x2000, &IoRegionId);

	// Register completion queue, requesting event-based notification for insertions by the kernel.
	CompletionQueue = (RIO_COMPLETION_QUEUE*) VirtualAlloc(NULL, sizeof(RIO_COMPLETION_QUEUE) + CQ_RING_SZ * sizeof(RIO_COMPLETION_ENTRY), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	RIO_NOTIFY Notify;
	Notify.Type = RIO_NOTIFY_EVENT;
	Notify.Event.EventHandle = (CompletionQueueEvent = CreateEvent(NULL, TRUE, FALSE, NULL));
	Notify.Event.NotifyReset = FALSE;

	Status = NxRegisterCompletionRing(CompletionQueue, CQ_RING_SZ, &Notify, &CompletionRingId);

	// Register request queue pair, routing notifications for both rx/tx to our singular completion queue.
	TxRequestQueue = (RIO_REQUEST_QUEUE*) VirtualAlloc(NULL, sizeof(RIO_REQUEST_QUEUE) + sizeof(RIO_REQUEST_ENTRY) * TX_RING_SZ, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	RxRequestQueue = (RIO_REQUEST_QUEUE*) VirtualAlloc(NULL, sizeof(RIO_REQUEST_QUEUE) + sizeof(RIO_REQUEST_ENTRY) * RX_RING_SZ, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);


	Status = NxRegisterRequestRing(Socket, TxRequestQueue, TX_RING_SZ, CompletionRingId, RxRequestQueue, RX_RING_SZ, CompletionRingId, NULL);



	RIO_REQUEST_ENTRY& Entry = TxRequestQueue->Ring[TxRequestQueue->Hdr.Head];


#define DATAGRAM_DATA  "TestDat"



	auto pd = IoRegion;

	memcpy(pd, DATAGRAM_DATA, sizeof(DATAGRAM_DATA));

	Entry.Data.RegionId = IoRegionId;
	Entry.Data.Offset = pd - IoRegion;
	Entry.Data.Length = sizeof(DATAGRAM_DATA);
	pd += Entry.Data.Length;

	sa.sin_addr.s_addr = 0x08080808;

	memcpy(pd, &sa, sizeof(sa));

	Entry.RemoteAddr.RegionId = IoRegionId;
	Entry.RemoteAddr.Offset = pd - IoRegion;
	Entry.RemoteAddr.Length = 28;
	pd += Entry.RemoteAddr.Length;

	TxRequestQueue->Hdr.Tail++;

	Status = NxPokeTx(Socket);

	Sleep(1000);
}

void main()
{


	// Currently NAPI only exposes support for IOCP-based IRP completion notification, as the expected use-case is 
	// for server software.
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);


	//DemoAccept();
	DemoConnect();

	//RioDemo();
	


}