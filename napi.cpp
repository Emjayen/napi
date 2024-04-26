/*
 * napi.cpp
 *
 */
#include "napi.h"
#include "afd.h"




// Handle to the internal RIO device child.
static HANDLE RioRegDomain;




/*
 * NxSocket
 *
 */
#define GET_PROTO_TYPE(proto) (proto >> 16)
#define GET_PROTO_PROTO(proto) (proto & 0xFFFF)


NTSTATUS NxSocket
(
	USHORT Family,
	ULONG Protocol,
	HANDLE* hSocketHandle
)
{
	UNICODE_STRING DevName;
	OBJECT_ATTRIBUTES ObjAttr;
	AFD_CREATE AfdCreate;
	IO_STATUS_BLOCK IoStatus;



	RtlInitUnicodeString(&DevName, AFD_DEVICE_NAME);
	InitializeObjectAttributes(&ObjAttr, &DevName, OBJ_CASE_INSENSITIVE | OBJ_INHERIT, 0, 0);

	AfdCreate.Ea.NextEntryOffset = 0;
	AfdCreate.Ea.Flags = 0;
	AfdCreate.Ea.EaNameLength = sizeof(AfdOpenPacket)-1;
	AfdCreate.Ea.EaValueLength = 30;
	memcpy(AfdCreate.Ea.EaName, AfdOpenPacket, sizeof(AfdOpenPacket));

	AfdCreate.EndpointFlags = 0;
	AfdCreate.GroupID = 0;
	AfdCreate.AddressFamily = Family;
	AfdCreate.SocketType = GET_PROTO_TYPE(Protocol);
	AfdCreate.Protocol = GET_PROTO_PROTO(Protocol);
	AfdCreate.SizeOfTdiName = 0;

	return NtCreateFile(hSocketHandle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &ObjAttr, &IoStatus, NULL, NULL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF, 0, &AfdCreate, sizeof(AfdCreate));
}






/*
 * NxBind
 *
 */
NTSTATUS NxBind
(
	HANDLE hSocket,
	sockaddr* Address,
	ULONG AddressLength,
	ULONG Share
)
{
	AFD_BIND_DATA Bind;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS Status;

	Bind.ShareType = Share;
	memcpy(Bind.AddressData, Address, AddressLength);

	if((Status = NtDeviceIoControlFile(hSocket, NULL, NULL, NULL, &IoStatus, IOCTL_AFD_BIND, &Bind, sizeof(ULONG) + AddressLength, Address, AddressLength)) == STATUS_PENDING)
		Status = IoStatus.Status;

	return Status;
}


/*
 * NxListen
 *
 */
NTSTATUS NxListen
(
	HANDLE hSocket,
	ULONG Backlog
)
{
	AFD_LISTEN_DATA Listen;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS Status;


	Listen.UseSAN = FALSE;
	Listen.Backlog = Backlog;
	Listen.UseDelayedAcceptance = FALSE;

	if((Status = NtDeviceIoControlFile(hSocket, NULL, NULL, NULL, &IoStatus, IOCTL_AFD_START_LISTEN, &Listen, sizeof(Listen), NULL, NULL)) == STATUS_PENDING)
		Status = IoStatus.Status;

	return Status;
}


/*
 * NxConnect
 *
 */
NTSTATUS NxConnect
(
	HANDLE hSocket,
	IO_STATUS_BLOCK* IoStatus,
	sockaddr* Address,
	USHORT AddressLength
)
{
	AFD_CONNECTEX_DATA Connect;

	Connect.UseSAN = FALSE;
	Connect.Tdi = TRUE;
	Connect.TdiAddressLength = AddressLength - 2;
	memcpy(Connect.AddressData, Address, AddressLength);

	IoStatus->Status = STATUS_PENDING;

	return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_CONNECTEX, &Connect, sizeof(Connect), NULL, NULL);
}


/*
 * NxAccept
 *
 */
NTSTATUS NxAccept
(
	HANDLE hSocket,
	IO_STATUS_BLOCK* IoStatus,
	HANDLE hAcceptSocket,
	PVOID Buffer,
	ULONG BufferLength
)
{
	AFD_ACCEPTEX_DATA Accept;

	Accept.UseSAN = FALSE;
	Accept.Unk = TRUE;
	Accept.AcceptSocket = hAcceptSocket;
	Accept.ReceiveDataLength = BufferLength - MAX_ENDPOINT_ADDRESS_SZ;
	Accept.LocalAddressLength = MAX_NETWORK_ADDRESS_SZ;
	Accept.RemoteAddressLength = MAX_NETWORK_ADDRESS_SZ;

	IoStatus->Status = STATUS_PENDING;

	return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_ACCEPTEX, &Accept, sizeof(Accept), Buffer, BufferLength);
}


/*
 * NxSend
 *
 */
NTSTATUS NxSend
(
	HANDLE hSocket,
	IO_STATUS_BLOCK* IoStatus,
	NETBUF* Buffers,
	ULONG BufferCount
)
{
	AFD_SEND_DATA Send;

	Send.Buffers = Buffers;
	Send.BufferCount = BufferCount;
	Send.AfdFlags = AFD_OVERLAPPED;
	Send.TdiFlags = 0;
	Send.Unused = 0;

	IoStatus->Status = STATUS_PENDING;

	return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_SEND, &Send, sizeof(Send), NULL, NULL);
}


/*
 * NxReceive
 *
 */
NTSTATUS NxReceive
(
	HANDLE hSocket,
	IO_STATUS_BLOCK* IoStatus,
	NETBUF* Buffers,
	ULONG BufferCount,
	ULONG Flags
)
{
	AFD_RECV_DATA Recv;
	
	Recv.Buffers = Buffers;
	Recv.BufferCount = BufferCount;
	Recv.AfdFlags = AFD_OVERLAPPED;
	Recv.TdiFlags = TDI_RECEIVE_NORMAL | Flags;
	Recv.Unused = 0;

	IoStatus->Status = STATUS_PENDING;

	return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_RECV, &Recv, sizeof(Recv), NULL, NULL);
}



/*
 * NxSetOption
 *
 */
#define OPT_GET_LEVEL(opt) (opt >> 16)
#define OPT_GET_NAME(opt) (opt & 0xFFFF)

NTSTATUS NxSetOption
(
	HANDLE hSocket,
	ULONG Option,
	ULONG Value
)
{
	union
	{
		AFD_TDI_OPT_DATA TdiOpt;
		AFD_INFO_DATA Info;
		BYTE Input[1];
	};

	union
	{
		IO_STATUS_BLOCK IoStatus;

		struct
		{
			ULONG Ioctl;
			ULONG InputLength;
		};
	};

	NTSTATUS Status;

	switch(Option)
	{
		case SO_RCVBUF:
		case SO_SNDBUF:
		{
			Info.InformationType = Option == SO_RCVBUF ? AFD_RECEIVE_WINDOW_SIZE : AFD_SEND_WINDOW_SIZE;
			Info.Information.Ulong = Value;

			Ioctl = IOCTL_AFD_SET_INFO;
			InputLength = sizeof(Info);
		} break;

		default:
		{
			if(Option == TCP_NAGLE)
				Value = ~Value;

			TdiOpt.Unk1 = 1;
			TdiOpt.Level = OPT_GET_LEVEL(Option);
			TdiOpt.Name = OPT_GET_NAME(Option);
			TdiOpt.Unk2 = 1;
			TdiOpt.Value = &Value;
			TdiOpt.Length = sizeof(ULONG);

			Ioctl = IOCTL_AFD_SET_TDI_OPT;
			InputLength = sizeof(TdiOpt);
		}
	}

	if((Status = NtDeviceIoControlFile(hSocket, NULL, NULL, NULL, &IoStatus, Ioctl, Input, InputLength, NULL, NULL)) == STATUS_PENDING)
		Status = IoStatus.Status;

	return Status;
}


#define AFD_RIO_CREATE_PACKET  "AfdRioRDOpenPacket"


/*
 * NxEnableRegisteredIo
 *
 */
NTSTATUS NxEnableRegisteredIo()
{
	IO_STATUS_BLOCK IoStatus;
	UNICODE_STRING DevName;
	OBJECT_ATTRIBUTES ObjAttr;
	
	union
	{
		FILE_FULL_EA_INFORMATION Ea;
		BYTE __pad[sizeof(Ea)-1 + sizeof(AFD_RIO_CREATE_PACKET)];
	} AfdCreate;

	RtlInitUnicodeString(&DevName, AFD_RIO_DEVICE_NAME);
	InitializeObjectAttributes(&ObjAttr, &DevName, OBJ_CASE_INSENSITIVE | OBJ_INHERIT, 0, 0);

	AfdCreate.Ea.NextEntryOffset = 0;
	AfdCreate.Ea.Flags = 0;
	AfdCreate.Ea.EaNameLength = sizeof(AFD_RIO_CREATE_PACKET)-1;
	AfdCreate.Ea.EaValueLength = 0;
	memcpy(AfdCreate.Ea.EaName, AFD_RIO_CREATE_PACKET, sizeof(AFD_RIO_CREATE_PACKET));

	return NtCreateFile(&RioRegDomain, GENERIC_READ | GENERIC_WRITE, &ObjAttr, &IoStatus, NULL, NULL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF, 0, &AfdCreate, sizeof(AfdCreate));
}


/*
 * NxRegisterIoRegion
 *
 */
NTSTATUS NxRegisterIoRegion
(
	PVOID RegionBase,
	ULONG Size,
	ULONG* RegionId
)
{
	AFD_RIO_REGISTER_IN Input;
	IO_STATUS_BLOCK IoStatus;

	Input.Operation = AFD_RIO_REGISTER;
	Input.RegionBaseAddress = RegionBase;
	Input.RegionSize = Size;

	return NtDeviceIoControlFile(RioRegDomain, NULL, NULL, NULL, &IoStatus, IOCTL_AFD_RIO, &Input, sizeof(Input), RegionId, sizeof(*RegionId));
}



/*
 * NxRegisterCompletionRing
 *
 */
NTSTATUS NxRegisterCompletionRing
(
	PVOID Ring,
	ULONG Size,
	RIO_NOTIFY* Notify,
	ULONG* RingId
)
{
	AFD_RIO_CQ_REGISTER_IN Input;
	IO_STATUS_BLOCK IoStatus;

	Input.Operation = AFD_RIO_CQ_REGISTER;
	Input.RingSize = Size;
	Input.NotificationType = Notify->Type;

	if(Input.NotificationType == RIO_NOTIFY_EVENT)
	{
		Input.Event.EventHandle = Notify->Event.EventHandle;
		Input.Event.NotifyReset = Notify->Event.NotifyReset;
	}

	else
	{
		Input.IOCP.PortHandle = Notify->IOCP.PortHandle;
		Input.IOCP.Context = Notify->IOCP.Context;
		Input.IOCP.IoStatusBlock = Notify->IOCP.IoStatusBlock;
	}

	Input.AllocationSize = sizeof(RIO_RING) + Input.RingSize * sizeof(RIO_COMPLETION_ENTRY);
	Input.Ring = Ring;

	return NtDeviceIoControlFile(RioRegDomain, NULL, NULL, NULL, &IoStatus, IOCTL_AFD_RIO, &Input, sizeof(Input), RingId, sizeof(*RingId));
}

/*
 * NxRegisterRequestRing
 * 
 */
NTSTATUS NxRegisterRequestRing
(
	HANDLE Socket,
	PVOID TxRing,
	ULONG TxRingSize,
	ULONG TxCompletionRingId,
	PVOID RxRing,
	ULONG RxRingSize,
	ULONG RxCompletionRingId,
	PVOID Context
)
{
	AFD_RIO_RQ_REGISTER_IN Input;
	IO_STATUS_BLOCK IoStatus;

	Input.Operation = AFD_RIO_RQ_REGISTER;
	Input.SendCompletionQueueId = TxCompletionRingId;
	Input.ReceiveCompletionQueueId = RxCompletionRingId;
	Input.SendQueueSize = TxRingSize;
	Input.SendAllocationSize = sizeof(RIO_RING) + TxRingSize * sizeof(RIO_REQUEST_ENTRY);
	Input.SendRingPtr = TxRing;
	Input.ReceiveQueueSize = RxRingSize;
	Input.ReceiveAllocationSize = sizeof(RIO_RING) + RxRingSize * sizeof(RIO_REQUEST_ENTRY);
	Input.ReceiveRingPtr = RxRing;
	Input.RioDomainHandle = RioRegDomain;
	Input.Context = Context;

	return NtDeviceIoControlFile(Socket, NULL, NULL, NULL, &IoStatus, IOCTL_AFD_RIO, &Input, sizeof(Input), 0, 0);
}