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
NTSTATUS NxSocket
(
	USHORT Family,
	ULONG Protocol,
	ULONG Flags,
	HANDLE* hSocketHandle
)
{
	AFD_OPEN_IN AfdOpen;
	UNICODE_STRING DevName;
	OBJECT_ATTRIBUTES ObjAttr;
	IO_STATUS_BLOCK IoStatus;


	RtlInitUnicodeString(&DevName, AFD_DEVICE_NAME);
	InitializeObjectAttributes(&ObjAttr, &DevName, OBJ_CASE_INSENSITIVE | OBJ_INHERIT, 0, 0);

	AfdOpen.Ea.NextEntryOffset = 0;
	AfdOpen.Ea.Flags = 0;
	AfdOpen.Ea.EaNameLength = sizeof(AFD_OPEN_PACKET)-1;
	AfdOpen.Ea.EaValueLength = 30;
	memcpy(AfdOpen.Ea.EaName, AFD_OPEN_PACKET, sizeof(AFD_OPEN_PACKET));
	AfdOpen.GroupID = 0;
	AfdOpen.AddressFamily = Family;
	AfdOpen.SocketType = __SOCKET_PARAMS_GET_TYPE(Protocol);
	AfdOpen.Protocol = __SOCKET_PARAMS_GET_PROTO(Protocol);
	AfdOpen.SizeOfTdiName = 0;
	AfdOpen.EndpointFlags = 0;
	
	if(AfdOpen.SocketType != SOCK_STREAM)
		AfdOpen.EndpointFlags |= AfdEndpointTypeDatagram;
	
	if(Flags & SOCK_FLAG_RIO)
		AfdOpen.EndpointFlags |= AFD_ENDPOINT_FLAG_REGISTERED_IO;
	
	return NtCreateFile(hSocketHandle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &ObjAttr, &IoStatus, NULL, NULL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF, 0, &AfdOpen, sizeof(AfdOpen));
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
NTSTATUS NxSetOption
(
	HANDLE hSocket,
	ULONG Option,
	ULONG Value
)
{
	union
	{
		AFD_INFORMATION AfdInfo;
		AFD_TRANSPORT_IOCTL_IN TransportIoctl;
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
		
	switch(Option)
	{
		case SO_RCVBUF:
		case SO_SNDBUF:
		{
			AfdInfo.InformationType = Option == SO_RCVBUF ? AFD_RECEIVE_WINDOW_SIZE : AFD_SEND_WINDOW_SIZE;
			AfdInfo.Information.Ulong = Value;

			Ioctl = IOCTL_AFD_SET_INFO;
			InputLength = sizeof(AfdInfo);
		} break;

		default:
		{
			TransportIoctl.Mode = AFD_TLI_WRITE;
			TransportIoctl.Level = __SOCKET_OPTION_GET_LEVEL(Option);
			TransportIoctl.IoControlCode = __SOCKET_OPTION_GET_NAME(Option);
			TransportIoctl.Flag = TRUE;
			TransportIoctl.InputBuffer = &Value;
			TransportIoctl.InputLength = sizeof(ULONG);

			Ioctl = IOCTL_AFD_TRANSPORT_IOCTL;
			InputLength = sizeof(TransportIoctl);
		}
	}

	return NtDeviceIoControlFile(hSocket, NULL, NULL, NULL, &IoStatus, Ioctl, Input, InputLength, NULL, NULL);
}


/*
 * NxIoControl
 *
 */
NTSTATUS NxIoControl
(
	HANDLE Socket,
	IO_STATUS_BLOCK* IoStatus,
	ULONG IoControlCode,
	PVOID InputBuffer,
	ULONG InputLength,
	PVOID OutputBuffer,
	ULONG OutputLength,
	ULONG* OutputReturnedLength
)
{
	AFD_TRANSPORT_IOCTL_IN Input;
	IO_STATUS_BLOCK ISB;
	NTSTATUS Status;

	if(!IoStatus)
		IoStatus = &ISB;

	Input.Mode = AFD_TLI_READ | AFD_TLI_WRITE;
	Input.Level = 0;
	Input.IoControlCode = IoControlCode;
	Input.Flag = TRUE;
	Input.InputBuffer = InputBuffer;
	Input.InputLength = InputLength;

	IoStatus->Status = STATUS_PENDING;

	Status = NtDeviceIoControlFile(Socket, NULL, NULL, NULL, IoStatus, AFD_TRANSPORT_IOCTL, &Input, sizeof(Input), OutputBuffer, OutputLength);

	if(OutputReturnedLength)
		*OutputReturnedLength = IoStatus->Information;

	return Status;
}


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
		BYTE __pad[sizeof(Ea) - alignof(FILE_FULL_EA_INFORMATION) + sizeof(AFD_RIO_OPEN_PACKET)];
	} AfdCreate;

	RtlInitUnicodeString(&DevName, AFD_RIO_DEVICE_NAME);
	InitializeObjectAttributes(&ObjAttr, &DevName, OBJ_CASE_INSENSITIVE | OBJ_INHERIT, 0, 0);

	AfdCreate.Ea.NextEntryOffset = 0;
	AfdCreate.Ea.Flags = 0;
	AfdCreate.Ea.EaNameLength = sizeof(AFD_RIO_OPEN_PACKET)-1;
	AfdCreate.Ea.EaValueLength = 0;
	memcpy(AfdCreate.Ea.EaName, AFD_RIO_OPEN_PACKET, sizeof(AFD_RIO_OPEN_PACKET));

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


/*
 * NxPokeTx
 *
 */
NTSTATUS NxPokeTx(HANDLE Socket)
{
	AFD_RIO_POKE_IN Input;
	IO_STATUS_BLOCK IoStatus;

	Input.Operation = AFD_RIO_TX_POKE;

	return NtDeviceIoControlFile(Socket, NULL, NULL, NULL, &IoStatus, IOCTL_AFD_RIO, &Input, sizeof(Input), 0, 0);
}


/*
 * NxPokeRx
 *
 */
NTSTATUS NxPokeRx(HANDLE Socket)
{
	AFD_RIO_POKE_IN Input;
	IO_STATUS_BLOCK IoStatus;

	Input.Operation = AFD_RIO_RX_POKE;

	return NtDeviceIoControlFile(Socket, NULL, NULL, NULL, &IoStatus, IOCTL_AFD_RIO, &Input, sizeof(Input), 0, 0);
}