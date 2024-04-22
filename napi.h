/*
 * napi.h
 *
 */
#pragma once
#include "nt/nt.h"





 // Protocols.
#define MAKE_SOCKET_PROTOCOL(type, proto) ((((ULONG) type) << 16) | ((ULONG) proto))

#define AF_INET  2

#define SOCK_STREAM  1
#define SOCK_DGRAM   2

#define SOL_SOCKET   0xFFFF

#define IP_P_UDP  17
#define IP_P_TCP  6

#define IPPROTO_UDP  MAKE_SOCKET_PROTOCOL(SOCK_DGRAM, IP_P_UDP)
#define IPPROTO_TCP  MAKE_SOCKET_PROTOCOL(SOCK_STREAM, IP_P_TCP)

// Socket options
#define MAKE_SOCKET_OPTION(level, option) ((((ULONG) level) << 16) | ((ULONG) option))

// SOL_SOCKET
#define SO_SNDBUF MAKE_SOCKET_OPTION(SOL_SOCKET, 0x1001)
#define SO_RCVBUF MAKE_SOCKET_OPTION(SOL_SOCKET, 0x1002)

// IPPROTO_TCP
#define TCP_NAGLE MAKE_SOCKET_OPTION(IPPROTO_TCP, 0x0001)

// Do not reuse address if already in use but allow subsequent reuse by others
#define BIND_SHARE_NORMAL	  0

// Reuse address if necessary
#define BIND_SHARE_REUSE	  1

// Address is a wildcard, no checking.
#define BIND_SHARE_WILDCARD   2

// Do not allow reuse of this address (admin only).
#define BIND_SHARE_EXCLUSIVE  3

// Reserved trailing buffer size for address control information returned from NxAccept operations.
#define MAX_NETWORK_ADDRESS_SZ   32
#define MAX_ENDPOINT_ADDRESS_SZ  (MAX_NETWORK_ADDRESS_SZ*2)


// Network address
struct sockaddr
{
	USHORT Family;
	BYTE Data[14];
};

struct sockaddr_in
{
	USHORT Family;
	USHORT Port;
	ULONG Address;
	BYTE Zero[8];
};

// SGIO buffer descriptor
struct NETBUF
{
	ULONG Length;
	VOID* Data;
};


/*
 * NxSocket
 *
 */
NTSTATUS NxSocket
(
	USHORT Family, 
	ULONG Protocol, 
	HANDLE* hSocketHandle
);


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
);


/*
 * NxListen
 *
 */
NTSTATUS NxListen
(
	HANDLE hSocket, 
	ULONG Backlog
);


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