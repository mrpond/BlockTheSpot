#pragma once

typedef int (WSAAPI* _getaddrinfo)(
	_In_opt_	const char* nodename,
	_In_opt_	const char* servname,
	_In_opt_	const struct addrinfo* hints,
	_Out_		struct addrinfo** res
	);

int WSAAPI getaddrinfo_hook (
	_In_opt_	const char* nodename,
	_In_opt_	const char* servname,
	_In_opt_	const struct addrinfo* hints,
	_Out_		struct addrinfo** res);

//#endif /* _HOSTS_H */
