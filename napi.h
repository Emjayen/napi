/*
 * napi.h
 *
 */
#pragma once
#pragma warning(disable:4200)
#include "nt/nt.h"
#include <ws2def.h>
#include <mstcpip.h>
#include <ws2tcpip.h>





//
// Socket types
//
#define __MAKE_SOCKET_PARAMS(type, proto) ((ULONG(type) << 16) | ULONG(proto))
#define __SOCKET_PARAMS_GET_TYPE(params) (params >> 16)
#define __SOCKET_PARAMS_GET_PROTO(params) (params & 0xFFFF)
#define SOCK_UDP __MAKE_SOCKET_PARAMS(SOCK_DGRAM, IPPROTO_UDP)
#define SOCK_TCP __MAKE_SOCKET_PARAMS(SOCK_STREAM, IPPROTO_TCP)


//
// Socket options
//
#define __MAKE_SOCKET_OPTION(level, name) ((ULONG(level) << 16) | ULONG(name))
#define __SOCKET_OPTION_GET_LEVEL(params) (params >> 16)
#define __SOCKET_OPTION_GET_NAME(params) (params & 0xFFFF)

// SOL_SOCKET
#undef SO_SNDBUF
#define SO_SNDBUF __MAKE_SOCKET_OPTION(SOL_SOCKET, 0x1001)
#undef SO_RCVBUF
#define SO_RCVBUF __MAKE_SOCKET_OPTION(SOL_SOCKET, 0x1002)

// IPPROTO_TCP
#undef TCP_NODELAY
#define TCP_NODELAY  MAKE_SOCKET_OPTION(IPPROTO_TCP, 0x0001)


//
// Bind dispositions
//
#define BIND_SHARE_NORMAL	  0 /* Do not reuse address if already in use but allow subsequent reuse by others */
#define BIND_SHARE_REUSE	  1 /* Reuse address if necessary */
#define BIND_SHARE_WILDCARD   2 /* Address is a wildcard, no checking. */
#define BIND_SHARE_EXCLUSIVE  3 /* Do not allow reuse of this address (elevated token required). */

// Reserved trailing buffer size for address control information returned from NxAccept operations.
#define MAX_NETWORK_ADDRESS_SZ   32
#define MAX_ENDPOINT_ADDRESS_SZ  (MAX_NETWORK_ADDRESS_SZ*2)

// Socket creation flags
#define SOCK_FLAG_RIO  (1<<0) /* Enable support for Registered I/O with this socket. */


//
// Registered I/O
//

// Completion notification parameters
#define RIO_NOTIFY_EVENT  1
#define RIO_NOTIFY_PORT   2

struct RIO_NOTIFY
{
	ULONG Type;

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
			PVOID Context;
			IO_STATUS_BLOCK* IoStatusBlock;
		} IOCP;
	};
};

// Buffer descriptor
struct RIO_BUF
{
	ULONG RegionId;
	ULONG Offset;
	ULONG Length;
};

// Request ring entry
struct RIO_REQUEST_ENTRY
{
	RIO_BUF Data;
	RIO_BUF LocalAddr;
	RIO_BUF RemoteAddr;
	RIO_BUF Control;
	RIO_BUF ReceiveFlags;
	ULONG Flags;
	PVOID64 Context;
};

// Completion ring entry
struct RIO_COMPLETION_ENTRY
{
	ULONG Status;
	ULONG Information;
	PVOID64 Context;
	PVOID64 RequestContext;
};

// Ring control area
__declspec(align(16)) struct RIO_RING
{
	ULONG Head;
	ULONG Tail;
};

// Request ring layout.
struct RIO_REQUEST_QUEUE
{
	RIO_RING Hdr;
	RIO_REQUEST_ENTRY Ring[];
};

// Completion ring layout
struct RIO_COMPLETION_QUEUE
{
	RIO_RING Hdr;
	RIO_COMPLETION_ENTRY Ring[];
};

// SGIO buffer descriptor
struct NETBUF
{
	ULONG Length;
	VOID* Data;
};


/*
 * NxSocket
 *    Creates a socket.
 * 
 * Parameters:
 *    [in]  'Family'       -- Specifies the address family; see AF_*
 *    [in]  'Protocol'     -- Specifies the transport protocol; see IP_PROTO_*
 *    [in]  'Flags'        -- Various flags; see SOCK_FLAG_*
 *    [out] 'SocketHandle' -- Receives the resulting socket on success. Undefined on failure.
 *
 * Return:
 *    On success returns STATUS_SUCCESS, elsewise returns an error status code.
 * 
 */
NTSTATUS NxSocket
(
	USHORT Family, 
	ULONG Protocol,
	ULONG Flags,
	HANDLE* SocketHandle
);


/*
 * NxBind
 *   Bind socket to local address.
 * 
 * Parameters:
 *   [in] 'Socket'        -- Handle to the socket to be bound.
 *   [in] 'Address'       -- Pointer to the address of which to bind the socket to.
 *   [in] 'AddressLength' -- Length of the address pointed to by 'Address'.
 *   [in] 'Share'         -- Address sharing mode; see: BIND_SHARE_*
 * 
 * Return:
 *    On success the return value is zero, elsewise on failure the return value is a status
 *    code indicating the error condition.
 *    
 */
NTSTATUS NxBind
(
	HANDLE Socket, 
	sockaddr* Address, 
	ULONG AddressLength, 
	ULONG Share
);


/*
 * NxListen
 *   Enable a connection-oriented socket to begin accepting connections. The socket must
 *   be bound prior to this call.
 * 
 * Parameters:
 *   [in] 'Socket'  -- Handle to the socket to begin listening.
 *   [in] 'Backlog' -- Hint of the internal maximum size of the accept queue. May be zero.
 * 
 * Remarks:
 *   Similarly to socket buffers, the accept queue functions as a fallback for kernel socket
 *   allocations that cannot be serviced by a pending user accept operation.
 *
 * Return:
 *    On success returns STATUS_SUCCESS, elsewise returns an error status code.
 *
 */
NTSTATUS NxListen
(
	HANDLE hSocket, 
	ULONG Backlog
);


/*
 * NxConnect
 *   Initiate connection to remote endpoint.
 * 
 * Parameters:
 *   [in] 'Socket' -- The socket to connect.
 *   [in] 'IoStatus' -- 
 */
NTSTATUS NxConnect
(
	HANDLE hSocket, 
	IO_STATUS_BLOCK* IoStatus, 
	sockaddr* Address, 
	USHORT AddressLength
);


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
);


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
);


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
);


/*
 * NxSetOption
 *
 */
NTSTATUS NxSetOption
(
	HANDLE hSocket,
	ULONG Option,
	ULONG Value
);


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
);


/*
 * NxEnableRegisteredIo
 *   Required one-shot initialization for subsequent usage of the Registered I/O (RIO) API.
 * 
 * Parameters:
 *   None.
 * 
 * Return:
 *    On success returns STATUS_SUCCESS, elsewise returns an error status code.
 *   
 */
NTSTATUS NxEnableRegisteredIo();


/*
 * NxRegisterIoRegion
 *    Registers a region of memory to enable it to be used as the source/destination for buffers passed
 *    to RIO send/receive operations.
 * 
 * Parameters:
 *    [in]  'RegionBase' -- Pointer to the base address of the region to be registered.
 *    [in]  'Size'       -- Size of the region, in bytes.
 *    [out] 'RegionId'   -- Receives an identifier that uniquely identifies the region.
 * 
 * Return:
 *    On success returns STATUS_SUCCESS, elsewise returns an error status code.
 *
 */
NTSTATUS NxRegisterIoRegion
(
	PVOID RegionBase,
	ULONG Size,
	ULONG* RegionId
);


/*
 * NxRegisterCompletionRing
 *   Registers a ring which will receive I/O completion notifications. 
 * 
 * Parameters:
 *   [in]  'Ring'   -- A pointer to the region of memory which will serve as the completion ring.
 *   [in]  'Size'   -- Size of the ring, in entries.
 *   [in]  'Notify' -- Specifies parameters which control the mechanism by which the kernel notifies of entry insertion.
 *   [out] 'RingId' -- Receives an identifier that uniquely identifies the ring.
 * 
 * Return:
 *    On success returns STATUS_SUCCESS, elsewise returns an error status code.
 *
 */
NTSTATUS NxRegisterCompletionRing
(
	PVOID Ring,
	ULONG Size,
	RIO_NOTIFY* Notify,
	ULONG* RingId
);


/*
 * NxRegisterRequestRing
 *    Registers a pair of rings for submitting send and receive operations on a socket.
 * 
 * Parameters:
 *    [in] 'Socket'                  -- The socket for which the rings are to be bound to.
 *    [in, opt] 'TxRing'             -- A pointer to the ring to be used for queuing send operations.
 *    [in, opt] 'TxRingSize'         -- The size of 'TxRing', in entries.
 *    [in, opt] 'TxCompletionRingId' -- The completion ring which will receive notifications of send operation completion.
 *    [in, opt] 'RxRing'             -- A pointer to the ring to be used for queuing receive operations.
 *    [in, opt] 'RxRingSize'         -- The size of 'RxRing', in entries.
 *    [in, opt] 'RxCompletionRingId' -- The completion ring which will receive notifications of receive operation completion.
 *    [in, opt] 'Context'            -- User context associated with the socke,
 * 
 * Return:
 *    On success returns STATUS_SUCCESS, elsewise returns an error status code.
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
);


/*
 * NxPokeTx
 *   Submits accumulated send operations in the provided socket's transmit ring.
 *
 */
NTSTATUS NxPokeTx(HANDLE Socket);


/*
 * NxPokeRx
 *   Submits accumulated receive operations in the provided socket's receive ring.
 *
 */
NTSTATUS NxPokeRx(HANDLE Socket);