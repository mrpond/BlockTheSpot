#pragma once
#include <winsock2.h>
#include <Ws2tcpip.h>


using _getaddrinfo = int(WSAAPI*) (PCSTR, PCSTR, const ADDRINFOA* pHints, PADDRINFOA* ppResult);

int WSAAPI getaddrinfo_hook (
	_In_opt_	PCSTR nodename,
	_In_opt_	PCSTR servname,
	_In_opt_	const ADDRINFOA* hints,
	_Out_		PADDRINFOA* res);



