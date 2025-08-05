/*
 * afd.h
 *   Defines the interface to the Ancillary Function Driver (afd.sys)
 *
 */
#pragma once
#include "tdi.h"




struct AFD_TRANSPORT_IOCTL_IN
{
	ULONG Mode;
	ULONG Level;
	ULONG Name;
	ULONG Flag;
	PVOID InputBuffer;
	ULONGLONG InputLength;
};


#define AFD_TLI_WRITE  1
#define AFD_TLI_READ   2

// AFD functions
#define AFD_BIND			    0
#define AFD_CONNECT			    1
#define AFD_START_LISTEN	    2
#define AFD_WAIT_FOR_LISTEN     3
#define AFD_ACCEPT              4
#define AFD_RECEIVE             5
#define AFD_RECEIVE_DATAGRAM    6
#define AFD_SEND			    7
#define AFD_SEND_DATAGRAM       8
#define AFD_PARTIAL_DISCONNECT  10
#define AFD_SET_INFO            14
#define AFD_SUPER_ACCEPT        32
#define AFD_TRANSPORT_IOCTL     47
#define AFD_SUPER_CONNECT       49
#define AFD_RECEIVE_MESSAGE     51
#define AFD_RIO                 70


// AFD IOCTLs
#define FSCTL_AFD_BASE   FILE_DEVICE_NETWORK
#define _AFD_CONTROL_CODE(Function, Method) \
	((FSCTL_AFD_BASE)<<12 | (Function<<2) | Method)

#define IOCTL_AFD_BIND \
  _AFD_CONTROL_CODE(AFD_BIND, METHOD_NEITHER)

#define IOCTL_AFD_CONNECT \
  _AFD_CONTROL_CODE(AFD_CONNECT, METHOD_NEITHER)

#define IOCTL_AFD_START_LISTEN \
  _AFD_CONTROL_CODE(AFD_START_LISTEN, METHOD_NEITHER)

#define IOCTL_AFD_RIO \
  _AFD_CONTROL_CODE(AFD_RIO, METHOD_NEITHER)

#define IOCTL_AFD_RECEIVE \
  _AFD_CONTROL_CODE(AFD_RECEIVE, METHOD_NEITHER)

#define IOCTL_AFD_RECEIVE_DATAGRAM \
  _AFD_CONTROL_CODE(AFD_RECEIVE_DATAGRAM, METHOD_NEITHER)

#define IOCTL_AFD_SEND \
  _AFD_CONTROL_CODE(AFD_SEND, METHOD_NEITHER)

#define IOCTL_AFD_SEND_DATAGRAM \
  _AFD_CONTROL_CODE(AFD_SEND_DATAGRAM, METHOD_NEITHER)

#define IOCTL_AFD_SET_INFO \
  _AFD_CONTROL_CODE(AFD_SET_INFO, METHOD_NEITHER)

#define IOCTL_AFD_ACCEPTEX \
  _AFD_CONTROL_CODE(AFD_SUPER_ACCEPT, METHOD_NEITHER)

#define IOCTL_AFD_TRANSPORT_IOCTL \
  _AFD_CONTROL_CODE(AFD_TRANSPORT_IOCTL, METHOD_NEITHER)

#define IOCTL_AFD_CONNECTEX \
  _AFD_CONTROL_CODE(AFD_SUPER_CONNECT, METHOD_NEITHER)

#define IOCTL_AFD_RECEIVE_MESSAGE \
  _AFD_CONTROL_CODE(AFD_RECEIVE_MESSAGE, METHOD_NEITHER)

#define IOCTL_AFD_PARTIAL_DISCONNECT \
  _AFD_CONTROL_CODE(AFD_PARTIAL_DISCONNECT, METHOD_NEITHER)


// AFD Endpoint information
#define AFD_INLINE_MODE          0x01
#define AFD_NONBLOCKING_MODE     0x02
#define AFD_MAX_SEND_SIZE        0x03
#define AFD_SENDS_PENDING        0x04
#define AFD_MAX_PATH_SEND_SIZE   0x05
#define AFD_RECEIVE_WINDOW_SIZE  0x06
#define AFD_SEND_WINDOW_SIZE     0x07
#define AFD_CONNECT_TIME         0x08
#define AFD_CIRCULAR_QUEUEING    0x09
#define AFD_GROUP_ID_AND_TYPE    0x0A
#define AFD_GROUP_ID_AND_TYPE    0x0A
#define AFD_REPORT_PORT_UNREACHABLE 0x0B

// Socket types
#define AFD_TYPE_STREAM 1
#define AFD_TYPE_DGRAM  2

// I/O control modifiers
#define AFD_NO_FAST_IO      0x0001  // Always fail Fast IO on this request.
#define AFD_OVERLAPPED      0x0002  // Overlapped operation.
#define AFD_IMMEDIATE       0x0004

// Endpoint flags.
#define AFD_ENDPOINT_FLAG_CONNECTIONLESS	0x00000001
#define AFD_ENDPOINT_FLAG_MESSAGEMODE		0x00000010
#define AFD_ENDPOINT_FLAG_RAW			    0x00000100
#define AFD_ENDPOINT_FLAG_REGISTERED_IO     0x10000000

//
// Old AFD_ENDPOINT_TYPE mappings. Flags make things clearer at
// at the TDI level and after all Winsock2 switched to provider flags
// instead of socket type anyway (ATM for example needs connection oriented
// raw sockets, which can only be reflected by SOCK_RAW+SOCK_STREAM combination
// which does not exists).
//
#define AfdEndpointTypeStream			0
#define AfdEndpointTypeDatagram			(AFD_ENDPOINT_FLAG_CONNECTIONLESS | AFD_ENDPOINT_FLAG_MESSAGEMODE)
#define AfdEndpointTypeRaw				(AFD_ENDPOINT_FLAG_CONNECTIONLESS | AFD_ENDPOINT_FLAG_MESSAGEMODE | AFD_ENDPOINT_FLAG_RAW)
#define AfdEndpointTypeSequencedPacket	(AFD_ENDPOINT_FLAG_MESSAGEMODE)
#define AfdEndpointTypeReliableMessage	(AFD_ENDPOINT_FLAG_MESSAGEMODE)

//
// New multipoint semantics
//
#define AFD_ENDPOINT_FLAG_MULTIPOINT	    0x00001000
#define AFD_ENDPOINT_FLAG_CROOT			    0x00010000
#define AFD_ENDPOINT_FLAG_DROOT			    0x00100000

// AFD device name.
#define AFD_DEVICE_NAME  L"\\Device\\Afd\\Endpoint"
#define AFD_OPEN_PACKET "AfdOpenPacketXX"

// AFD RIO registration device name
#define AFD_RIO_DEVICE_NAME  L"\\Device\\Afd\\RioRegDomain"
#define AFD_RIO_OPEN_PACKET  "AfdRioRDOpenPacket"


struct AFD_OPEN_IN
{
	union
	{
		FILE_FULL_EA_INFORMATION Ea;
		BYTE EaPadding[sizeof(Ea) - alignof(FILE_FULL_EA_INFORMATION) + sizeof(AFD_OPEN_PACKET)];
	};

	ULONG EndpointFlags;
	ULONG GroupID;
	ULONG AddressFamily;
	ULONG SocketType;
	ULONG Protocol;
	ULONG SizeOfTdiName;
	CHAR TdiName[9];
};


struct AFD_BIND_IN
{
	ULONG ShareType;
	BYTE AddressData[28];
};


struct AFD_LISTEN_IN
{
	BOOL	UseSAN;
	ULONG	Backlog;
	BOOL	UseDelayedAcceptance;
};


struct AFD_CONNECT_IN
{
	BOOLEAN SanActive;
	HANDLE RootEndpoint;
	HANDLE ConnectEndpoint;
	BYTE AddressData[28];
};

struct AFD_SUPER_CONNECT_IN
{
	BOOL UseSAN;
	ULONG Tdi;
	USHORT TdiAddressLength;
	BYTE AddressData[28];
};

struct AFD_SUPER_ACCEPT_IN
{
	BOOLEAN UseSAN;
	BOOLEAN Unk; // TRUE
	HANDLE AcceptSocket;
	ULONG ReceiveDataLength;
	ULONG LocalAddressLength;
	ULONG RemoteAddressLength;
};

struct AFD_INFORMATION
{
	ULONG InformationType;

	union 
	{
		BOOLEAN Boolean;
		ULONG Ulong;
		LARGE_INTEGER LargeInteger;
	} Information;
};

struct AFD_SEND_IN
{
	NETBUF* Buffers;
	ULONG BufferCount;
	ULONG AfdFlags;
	ULONG TdiFlags;
	ULONG Unused;
};

struct AFD_SEND_DATAGRAM_IN
{
	NETBUF* Buffers;
	ULONG BufferCount;
	ULONG AfdFlags;
	TDI_REQUEST_SEND_DATAGRAM  TdiRequest;
	TDI_CONNECTION_INFORMATION  TdiConnInfo;
};

struct AFD_RECV_DATAGRAM_IN
{
	NETBUF* Buffers;
	ULONG BufferCount;
	ULONG AfdFlags;
	ULONG TdiFlags;
	PVOID Address;
	PULONG AddressLength;
};

struct AFD_RECV_IN
{
	NETBUF* Buffers;
	ULONG BufferCount;
	ULONG AfdFlags;
	ULONG TdiFlags;
	ULONG Unused;
};

struct AFD_RECEIVE_MESSAGE_IN
{
	AFD_RECV_DATAGRAM_IN Datagram;
	PVOID ControlBuffer;
	PULONG ControlBufferLength;
	PULONG MsgFlags;
};

struct AFD_TRANSMIT_PACKETS_IN
{
	NETBUFEX* Buffers;
	ULONG Count;
	ULONG SendSize;
	ULONG Flags;
};


struct AFD_PARTIAL_DISCONNECT_IN
{
	ULONG Flags;
	ULONG64 Timeout;
};

// Registered I/O Operations
#define AFD_RIO_CQ_REGISTER 0
#define AFD_RIO_CQ_CLOSE    1
#define AFD_RIO_NOTIFY      2
#define AFD_RIO_RQ_REGISTER 3
#define AFD_RIO_REGISTER    4
#define AFD_RIO_DEREGISTER  5
#define AFD_RIO_TX_POKE     6
#define AFD_RIO_RX_POKE     7


struct AFD_RIO_POKE_IN
{
	ULONG Operation;
};

struct AFD_RIO_REGISTER_IN
{
	ULONG Operation;
	PVOID64 RegionBaseAddress;
	ULONG RegionSize;
};

struct AFD_RIO_CQ_REGISTER_IN
{
	ULONG Operation;
	ULONG RingSize;
	ULONG NotificationType;

	union
	{
		struct
		{
			HANDLE EventHandle;
			BOOL NotifyReset;
		} Event;

		struct
		{
			HANDLE PortHandle;
			PVOID64 Context;
			PVOID64 IoStatusBlock;
		} IOCP;
	};
	
	ULONG AllocationSize;
	PVOID64 Ring;
};

struct AFD_RIO_RQ_REGISTER_IN
{
	ULONG Operation;
	ULONG SendCompletionQueueId;
	ULONG ReceiveCompletionQueueId;
	ULONG SendQueueSize;
	ULONG SendAllocationSize;
	PVOID64 SendRingPtr;
	ULONG ReceiveQueueSize;
	ULONG ReceiveAllocationSize;
	PVOID64 ReceiveRingPtr;
	PVOID64 RioDomainHandle;
	PVOID Context;
};

struct AFD_RIO_NOTIFY_IN
{
	ULONG Operation;
	ULONG CompletionQueueId;
};