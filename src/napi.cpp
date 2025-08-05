/*
 * napi.cpp
 *
 */
#include "napi.h"
#include "afd.h"




// Handle to the internal RIO device child.
static HANDLE RioRegDomain;




/*
 * IrpBusyWait
 *
 */
static NTSTATUS IrpBusyWait(IO_STATUS_BLOCK* ISB)
{
	// Note that the kernel is required to write the infomation field prior to the status.
	while(*((volatile NTSTATUS*) &ISB->Status) == STATUS_PENDING)
		_mm_pause();

	return ISB->Status;
}


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
	InitializeObjectAttributes(&ObjAttr, &DevName, OBJ_CASE_INSENSITIVE, 0, 0);

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
	
	return NtCreateFile(hSocketHandle, GENERIC_READ | GENERIC_WRITE, &ObjAttr, &IoStatus, NULL, NULL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF, 0, &AfdOpen, sizeof(AfdOpen));
}


/*
 * NxBind
 *
 */
NTSTATUS NxBind
(
	HANDLE hSocket,
	PVOID Address,
	ULONG AddressLength,
	ULONG Share
)
{
	AFD_BIND_IN Bind;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS Status;

	Bind.ShareType = Share;
	memcpy(Bind.AddressData, Address, AddressLength);

	if((Status = NtDeviceIoControlFile(hSocket, NULL, NULL, NULL, &IoStatus, IOCTL_AFD_BIND, &Bind, sizeof(ULONG) + AddressLength, Address, AddressLength)) == STATUS_PENDING)
		Status = IoStatus.Status;

	return Status;
}


/*
 * NxAssociate
 *
 */
NTSTATUS NxAssociate
(
	HANDLE hSocket,
	PVOID Address,
	ULONG AddressLength
)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK ISB;
	AFD_CONNECT_IN In;

	In.SanActive = FALSE;
	In.RootEndpoint = NULL;
	In.ConnectEndpoint = NULL;
	memcpy(In.AddressData, Address, AddressLength);

	if((Status = NtDeviceIoControlFile(hSocket, NULL, NULL, NULL, &ISB, IOCTL_AFD_CONNECT, &In, sizeof(In), NULL, 0)) == STATUS_PENDING)
		Status = IrpBusyWait(&ISB);

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
	AFD_LISTEN_IN Listen;
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
	PVOID Address,
	USHORT AddressLength
)
{
	AFD_SUPER_CONNECT_IN In;

	In.UseSAN = FALSE;
	In.Tdi = TRUE;
	In.TdiAddressLength = AddressLength - sizeof(In.TdiAddressLength);
	memcpy(In.AddressData, Address, AddressLength);
	
	IoStatus->Status = STATUS_PENDING;

	return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_CONNECTEX, &In, sizeof(In), NULL, NULL);
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
	AFD_SUPER_ACCEPT_IN Accept;

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
	AFD_SEND_IN In;

	In.Buffers = Buffers;
	In.BufferCount = BufferCount;
	In.AfdFlags = AFD_OVERLAPPED;
	In.TdiFlags = 0;
	In.Unused = 0;

	IoStatus->Status = STATUS_PENDING;

	return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_SEND, &In, sizeof(In), NULL, NULL);
}


/*
 * NxSendDatagram
 *
 */
NTSTATUS NxSendDatagram
(
	HANDLE hSocket,
	IO_STATUS_BLOCK* IoStatus,
	NETBUF* Buffers,
	ULONG BufferCount,
	PVOID DestinationAddress,
	ULONG DestinationAddressLength
)
{
	AFD_SEND_DATAGRAM_IN In = {};

	In.Buffers = Buffers;
	In.BufferCount = BufferCount;
	In.AfdFlags = AFD_OVERLAPPED;
	In.TdiConnInfo.RemoteAddress = DestinationAddress;
	In.TdiConnInfo.RemoteAddressLength = DestinationAddressLength;

	IoStatus->Status = STATUS_PENDING;

	return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_SEND_DATAGRAM, &In, sizeof(In), NULL, 0);
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
	AFD_RECV_IN Recv;
	
	Recv.Buffers = Buffers;
	Recv.BufferCount = BufferCount;
	Recv.AfdFlags = AFD_OVERLAPPED;
	Recv.TdiFlags = TDI_RECEIVE_NORMAL | Flags;
	Recv.Unused = 0;

	IoStatus->Status = STATUS_PENDING;

	return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_RECEIVE, &Recv, sizeof(Recv), NULL, NULL);
}


/*
 * NxReceiveDatagram
 *
 */
NTSTATUS NxReceiveDatagram
(
	HANDLE hSocket,
	IO_STATUS_BLOCK* IoStatus,
	NETBUF* Buffers,
	ULONG BufferCount,
	ULONG Flags,
	PVOID Address,
	ULONG* AddressLength
)
{
	AFD_RECV_DATAGRAM_IN In;

	In.Buffers = Buffers;
	In.BufferCount = BufferCount;
	In.AfdFlags = AFD_OVERLAPPED;
	In.TdiFlags = TDI_RECEIVE_NORMAL | Flags;
	In.Address = Address;
	In.AddressLength = AddressLength;

	IoStatus->Status = STATUS_PENDING;

	return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_RECEIVE_DATAGRAM, &In, sizeof(In), NULL, 0);
}

/*
 * NxReceiveMessage
 *
 */
NTSTATUS NxReceiveMessage
(
	HANDLE hSocket,
	IO_STATUS_BLOCK* IoStatus,
	NETBUF* Buffers,
	ULONG BufferCount,
	ULONG Flags,
	PVOID Address,
	ULONG* AddressLength,
	PVOID ControlBuffer,
	ULONG* ControlBufferLength,
	ULONG* MsgFlags
)
{
	AFD_RECEIVE_MESSAGE_IN In;

	In.Datagram.Buffers = Buffers;
	In.Datagram.BufferCount = BufferCount;
	In.Datagram.AfdFlags = AFD_OVERLAPPED;
	In.Datagram.TdiFlags = TDI_RECEIVE_NORMAL | Flags;
	In.Datagram.Address = Address;
	In.Datagram.AddressLength = AddressLength;
	In.ControlBuffer = ControlBuffer;
	In.ControlBufferLength = ControlBufferLength;
	In.MsgFlags = MsgFlags;

	IoStatus->Status = STATUS_PENDING;

	return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_RECEIVE_MESSAGE, &In, sizeof(In), NULL, 0);
}


/*
 * NxShutdown
 *
 */
NTSTATUS NxShutdown
(
	HANDLE hSocket,
	ULONG Flags
)
{
	AFD_PARTIAL_DISCONNECT_IN In;
	IO_STATUS_BLOCK IoStatus;


	In.Flags = Flags;

	// This appears to be an artifact of the NT4 days and is ignored by the [emulation] TDI layer
	// in the kernel. For documentation sake; Winsock does set it to various sentinel values:
	//
	// - During a shutdown call -1 is passed. 
	// - During an implicit abortive disconnect during handle closure zero is passed. This is translated
	//   at TDI to be zero, thus -1 and 0 are semantically equivilent.
	//
	In.Timeout = 0;

	return NtDeviceIoControlFile(hSocket, NULL, NULL, NULL, &IoStatus, IOCTL_AFD_PARTIAL_DISCONNECT, &In, sizeof(In), NULL, 0);
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
	NTSTATUS Status;

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
		case SOPT_RCVBUF:
		case SOPT_SNDBUF:
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
			TransportIoctl.Name = __SOCKET_OPTION_GET_NAME(Option);
			TransportIoctl.Flag = TRUE;
			TransportIoctl.InputBuffer = &Value;
			TransportIoctl.InputLength = sizeof(ULONG);

			Ioctl = IOCTL_AFD_TRANSPORT_IOCTL;
			InputLength = sizeof(TransportIoctl);
		}
	}

	Status = NtDeviceIoControlFile(hSocket, NULL, NULL, NULL, &IoStatus, Ioctl, Input, InputLength, NULL, NULL);

	if(Status == STATUS_PENDING)
		Status = IrpBusyWait(&IoStatus);

	return Status;
}


/*
 * NxGetOption
 *
 */
NTSTATUS NxGetOption(HANDLE hSocket, ULONG Option, ULONG* Value)
{
	NTSTATUS Status;

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
		/*case SOPT_RCVBUF:
		case SOPT_SNDBUF:
		{
			AfdInfo.InformationType = Option == SO_RCVBUF ? AFD_RECEIVE_WINDOW_SIZE : AFD_SEND_WINDOW_SIZE;
			AfdInfo.Information.Ulong = Value;

			Ioctl = IOCTL_AFD_SET_INFO;
			InputLength = sizeof(AfdInfo);
		} break;*/

		default:
		{
			TransportIoctl.Mode = AFD_TLI_READ;
			TransportIoctl.Level = __SOCKET_OPTION_GET_LEVEL(Option);
			TransportIoctl.Name = __SOCKET_OPTION_GET_NAME(Option);
			TransportIoctl.Flag = TRUE;
			TransportIoctl.InputBuffer = NULL;
			TransportIoctl.InputLength = 0;

			Ioctl = IOCTL_AFD_TRANSPORT_IOCTL;
			InputLength = sizeof(TransportIoctl);
		}
	}

	Status = NtDeviceIoControlFile(hSocket, NULL, NULL, NULL, &IoStatus, Ioctl, Input, InputLength, Value, sizeof(Value));

	if(Status == STATUS_PENDING)
		Status = IrpBusyWait(&IoStatus);

	return Status;
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
	Input.Name = IoControlCode;
	Input.Flag = TRUE;
	Input.InputBuffer = InputBuffer;
	Input.InputLength = InputLength;

	IoStatus->Status = STATUS_PENDING;

	Status = NtDeviceIoControlFile(Socket, NULL, NULL, NULL, IoStatus, IOCTL_AFD_TRANSPORT_IOCTL, &Input, sizeof(Input), OutputBuffer, OutputLength);

	if(IoStatus == &ISB && Status == STATUS_PENDING)
		Status = IrpBusyWait(IoStatus);

	if(OutputReturnedLength)
		*OutputReturnedLength = (ULONG) IoStatus->Information;

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
 * NxNotify
 *
 */
NTSTATUS NxNotify(ULONG CompletionQueueId)
{
	AFD_RIO_NOTIFY_IN Input;
	IO_STATUS_BLOCK IoStatus;

	Input.Operation = AFD_RIO_NOTIFY;
	Input.CompletionQueueId = CompletionQueueId;

	return NtDeviceIoControlFile(RioRegDomain, NULL, NULL, NULL, &IoStatus, IOCTL_AFD_RIO, &Input, sizeof(Input), 0, 0);
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