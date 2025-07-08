/*
 * tdi.h
 *   Defines the Transport Driver Interface (TDI)
 * 
 */
#pragma once





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



typedef LONG TDI_STATUS;
typedef PVOID CONNECTION_CONTEXT;


struct TDI_REQUEST
{
	union {
		HANDLE AddressHandle;
		CONNECTION_CONTEXT ConnectionContext;
		HANDLE ControlChannel;
	} Handle;

	PVOID RequestNotifyObject;
	PVOID RequestContext;
	TDI_STATUS TdiStatus;
};

struct TDI_CONNECTION_INFORMATION
{
	LONG UserDataLength;        // length of user data buffer
	PVOID UserData;             // pointer to user data buffer
	LONG OptionsLength;         // length of follwoing buffer
	PVOID Options;              // pointer to buffer containing options
	LONG RemoteAddressLength;   // length of following buffer
	PVOID RemoteAddress;        // buffer containing the remote address
};

struct TDI_REQUEST_SEND_DATAGRAM
{
	TDI_REQUEST Request;
	TDI_CONNECTION_INFORMATION* SendDatagramInformation;
};
