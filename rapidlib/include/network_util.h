#pragma once

#include "types_def.h"
#if RAPID_PLATFORM == RAPID_PLATFORM_WIN32
#include <WinSock2.h>
#else
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

namespace rapidlib
{

	class socket_address
	{
	public:
		void set(r_uint32 address, r_uint16 port)
		{
			m_binary_address = address;
			m_port = port;
		}

		void reset()
		{
			m_binary_address = 0xFFFFFFFF;
			m_port = 0xFFFF;
		}

		r_bool operator==(const socket_address& sa) const
		{
			return m_binary_address == sa.m_binary_address && m_port == sa.m_port;
		}
		r_bool operator!=(const socket_address& sa) const
		{
			return m_binary_address != sa.m_binary_address || m_port != sa.m_port;
		}
		r_bool operator<(const socket_address& sa) const
		{
			return m_binary_address < sa.m_binary_address || (m_binary_address == sa.m_binary_address && m_port < sa.m_port);
		}
		r_bool operator>(const socket_address& sa) const
		{
			return m_binary_address > sa.m_binary_address || (m_binary_address == sa.m_binary_address && m_port > sa.m_port);
		}

		inline r_uint32 get_binary_address() const { return m_binary_address; }
		inline r_uint16 get_port() const { return m_port; }

	private:
		r_uint32		m_binary_address = 0xFFFFFFFF;			// the address from inet_addr
		r_uint16		m_port = 0xFFFF;						// the port number
	};

	/**
	* @struct sock_packet
	* @brief This represents a user message from another system.
	*/
	typedef struct sock_packet {
		socket_address					mSystemAddress; /**< the system that send this packet. */
		r_uint32						mIndex;         /**< server-only, this is the index into the player array that this socketAddress maps to. */
		r_uint32						mLength;        /**< the length of the data in bytes. */
		unsigned char*					mData;          /**< the data from the sender. */

		enum packet_type
		{
			packet_connected,
			packet_disconnected,
			packet_message,
			packet_connect_server_failed,
		};
		packet_type						m_type;
	} sock_packet;


	class network_util
	{
	public:
		static r_uint32 from_address(const char* ip);
		static const char* from_address(r_uint32 ip, char* host, r_int32 len);
		static r_uint32 from_host(const char* host);
		static r_uint32 from_domain(const char* host);
	};
}