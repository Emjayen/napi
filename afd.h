/*
 * afd.h
 *   Internal. Defines the interface to AFD.
 *
 */
#pragma once



// AFD commands
#define AFD_BIND			0
#define AFD_CONNECT			1
#define AFD_START_LISTEN	2
#define AFD_RECV			5
#define AFD_SEND			7
#define AFD_SEND_DATAGRAM   8
#define AFD_SET_INFO        14
#define AFD_ACCEPTEX        32
#define AFD_SET_TDI_OPT     47 // Internally TdiSetInformationEx
#define AFD_CONNECTEX       49

// AFD IOCTLs
#define FSCTL_AFD_BASE   FILE_DEVICE_NETWORK
#define _AFD_CONTROL_CODE(Operation,Method) \
  ((FSCTL_AFD_BASE)<<12 | (Operation<<2) | Method)

#define IOCTL_AFD_BIND \
  _AFD_CONTROL_CODE(AFD_BIND, METHOD_NEITHER)

#define IOCTL_AFD_CONNECT \
  _AFD_CONTROL_CODE(AFD_CONNECT, METHOD_NEITHER)

#define IOCTL_AFD_START_LISTEN \
  _AFD_CONTROL_CODE(AFD_START_LISTEN, METHOD_NEITHER)

#define IOCTL_AFD_RECV \
  _AFD_CONTROL_CODE(AFD_RECV, METHOD_NEITHER)

#define IOCTL_AFD_SEND \
  _AFD_CONTROL_CODE(AFD_SEND, METHOD_NEITHER)

#define IOCTL_AFD_SET_INFO \
  _AFD_CONTROL_CODE(AFD_SET_INFO, METHOD_NEITHER)

#define IOCTL_AFD_ACCEPTEX \
  _AFD_CONTROL_CODE(AFD_ACCEPTEX, METHOD_NEITHER)

#define IOCTL_AFD_SET_TDI_OPT \
  _AFD_CONTROL_CODE(AFD_SET_TDI_OPT, METHOD_NEITHER)

#define IOCTL_AFD_CONNECTEX \
  _AFD_CONTROL_CODE(AFD_CONNECTEX, METHOD_NEITHER)


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

#define TDI_RECEIVE_BROADCAST           0x00000004 // received TSDU was broadcast.
#define TDI_RECEIVE_MULTICAST           0x00000008 // received TSDU was multicast.
#define TDI_RECEIVE_PARTIAL             0x00000010 // received TSDU is not fully presented.
#define TDI_RECEIVE_NORMAL              0x00000020 // received TSDU is normal data
#define TDI_RECEIVE_EXPEDITED           0x00000040 // received TSDU is expedited data
#define TDI_RECEIVE_PEEK                0x00000080 // received TSDU is not released
#define TDI_RECEIVE_NO_RESPONSE_EXP     0x00000100 // HINT: no back-traffic expected
#define TDI_RECEIVE_COPY_LOOKAHEAD      0x00000200 // for kernel-mode indications
#define TDI_RECEIVE_ENTIRE_MESSAGE      0x00000400 // opposite of RECEIVE_PARTIAL

//  (for kernel-mode indications)
#define TDI_RECEIVE_AT_DISPATCH_LEVEL   0x00000800 // receive indication called
												   //  at dispatch level
#define TDI_RECEIVE_CONTROL_INFO        0x00001000 // Control info is being passed up.
#define TDI_RECEIVE_FORCE_INDICATION    0x00002000 // reindicate rejected data.
#define TDI_RECEIVE_NO_PUSH             0x00004000 // complete only when full.

// Options which are used for both SendOptions and ReceiveIndicators.
#define TDI_SEND_EXPEDITED            ((USHORT) 0x0020) // TSDU is/was urgent/expedited.
#define TDI_SEND_PARTIAL              ((USHORT) 0x0040) // TSDU is/was terminated by an EOR.
#define TDI_SEND_NO_RESPONSE_EXPECTED ((USHORT) 0x0080) // HINT: no back traffic expected.
#define TDI_SEND_NON_BLOCKING         ((USHORT) 0x0100) // don't block if no buffer space in protocol
#define TDI_SEND_AND_DISCONNECT       ((USHORT) 0x0200) // Piggy back disconnect to remote and do not indicate disconnect from remote

// TDI address types
#define TDI_ADDRESS_TYPE_UNSPEC    ((USHORT) 0)  // unspecified
#define TDI_ADDRESS_TYPE_UNIX      ((USHORT) 1)  // local to host (pipes, portals)
#define TDI_ADDRESS_TYPE_IP        ((USHORT) 2)  // internetwork: UDP, TCP, etc.
#define TDI_ADDRESS_TYPE_IMPLINK   ((USHORT) 3)  // arpanet imp addresses
#define TDI_ADDRESS_TYPE_PUP       ((USHORT) 4)  // pup protocols: e.g. BSP
#define TDI_ADDRESS_TYPE_CHAOS     ((USHORT) 5)  // mit CHAOS protocols
#define TDI_ADDRESS_TYPE_NS        ((USHORT) 6)  // XEROX NS protocols
#define TDI_ADDRESS_TYPE_IPX       ((USHORT) 6)  // Netware IPX
#define TDI_ADDRESS_TYPE_NBS       ((USHORT) 7)  // nbs protocols
#define TDI_ADDRESS_TYPE_ECMA      ((USHORT) 8)  // european computer manufacturers
#define TDI_ADDRESS_TYPE_DATAKIT   ((USHORT) 9)  // datakit protocols
#define TDI_ADDRESS_TYPE_CCITT     ((USHORT) 10) // CCITT protocols, X.25 etc
#define TDI_ADDRESS_TYPE_SNA       ((USHORT) 11) // IBM SNA
#define TDI_ADDRESS_TYPE_DECnet    ((USHORT) 12) // DECnet
#define TDI_ADDRESS_TYPE_DLI       ((USHORT) 13) // Direct data link interface
#define TDI_ADDRESS_TYPE_LAT       ((USHORT) 14) // LAT
#define TDI_ADDRESS_TYPE_HYLINK    ((USHORT) 15) // NSC Hyperchannel
#define TDI_ADDRESS_TYPE_APPLETALK ((USHORT) 16) // AppleTalk
#define TDI_ADDRESS_TYPE_NETBIOS   ((USHORT) 17) // Netbios Addresses
#define TDI_ADDRESS_TYPE_8022      ((USHORT) 18) //
#define TDI_ADDRESS_TYPE_OSI_TSAP  ((USHORT) 19) //
#define TDI_ADDRESS_TYPE_NETONE    ((USHORT) 20) // for WzMail
#define TDI_ADDRESS_TYPE_VNS       ((USHORT) 21) // Banyan VINES IP
#define TDI_ADDRESS_TYPE_NETBIOS_EX ((USHORT) 22) // NETBIOS address extensions
#define TDI_ADDRESS_TYPE_IP6       ((USHORT) 23) // IP version 6
#define TDI_ADDRESS_TYPE_NETBIOS_UNICODE_EX       ((USHORT)24) // WCHAR Netbios address

// AFD multiplexed device name.
#define AFD_DEVICE_NAME  L"\\Device\\Afd\\Endpoint"

struct AFD_CREATE
{
#define AfdOpenPacket "AfdOpenPacketXX"

	// Inline FILE_FULL_EA_INFORMATION
	struct
	{
		ULONG NextEntryOffset;
		UCHAR Flags;
		UCHAR EaNameLength;
		USHORT EaValueLength;
		CHAR EaName[16];
	} Ea;

	ULONG EndpointFlags;
	ULONG GroupID;
	ULONG AddressFamily;
	ULONG SocketType;
	ULONG Protocol;
	ULONG SizeOfTdiName;
	CHAR TdiName[9];
};


struct AFD_BIND_DATA
{
	ULONG ShareType;
	BYTE AddressData[28];
};


struct AFD_LISTEN_DATA
{
	BOOL	UseSAN;
	ULONG	Backlog;
	BOOL	UseDelayedAcceptance;
};


struct AFD_CONNECTEX_DATA
{
	BOOL UseSAN;
	ULONG Tdi;
	USHORT TdiAddressLength;
	BYTE AddressData[28];
};

struct AFD_ACCEPTEX_DATA
{
	BOOLEAN UseSAN;
	BOOLEAN Unk; // TRUE
	HANDLE AcceptSocket;
	ULONG ReceiveDataLength;
	ULONG LocalAddressLength;
	ULONG RemoteAddressLength;
};

struct AFD_INFO_DATA
{
	ULONG InformationType;

	union 
	{
		BOOLEAN Boolean;
		ULONG Ulong;
		LARGE_INTEGER LargeInteger;
	} Information;
};

struct AFD_TDI_OPT_DATA
{
	BOOLEAN Unk1; // TRUE
	ULONG Level;
	ULONG Name;
	ULONG Unk2; // TRUE
	VOID* Value;
	ULONG Length;
};


struct AFD_SEND_DATA
{
	NETBUF* Buffers;
	ULONG BufferCount;
	ULONG AfdFlags;
	ULONG TdiFlags;
	ULONG Unused;
};


struct AFD_RECV_DATA
{
	NETBUF* Buffers;
	ULONG BufferCount;
	ULONG AfdFlags;
	ULONG TdiFlags;
	ULONG Unused;
};
