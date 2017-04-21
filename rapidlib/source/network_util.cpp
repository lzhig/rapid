#include "network_util.h"

#if (RAPID_PLATFORM == RAPID_PLATFORM_WIN32)
#include <WS2tcpip.h>
#else
#include <string.h>
#endif

namespace rapidlib
{
	r_uint32 network_util::from_address(const char * ip)
	{
		struct in_addr addr;
		auto ret = ::inet_pton(AF_INET, ip, &addr);
		if (ret <= 0)
			return INADDR_NONE;
		return addr.s_addr;
	}
	const char * network_util::from_address(r_uint32 ip, char* host, r_int32 len)
	{
		in_addr in;
		in.s_addr = ip;
		return ::inet_ntop(AF_INET, &in, host, len);
	}
	r_uint32 network_util::from_host(const char * host)
	{
		struct in_addr addr;
		auto ret = ::inet_pton(AF_INET, 
			host == nullptr || host[0] == '\0' ? "127.0.0.1" : host,
			&addr);

		return ret <= 0 ? from_domain(host) : addr.s_addr;
	}
	r_uint32 network_util::from_domain(const char * host)
	{
		struct addrinfo *result = nullptr;
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;


		auto ret = ::getaddrinfo(host, nullptr, &hints, &result);
		if (ret != 0 || result == nullptr)
			return INADDR_NONE;

		auto address = ((struct sockaddr_in*)result->ai_addr)->sin_addr.s_addr;

		freeaddrinfo(result);

		return address;
	}
}
