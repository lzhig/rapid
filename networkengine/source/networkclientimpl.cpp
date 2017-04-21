#include "networkclientimpl.h"
#include <stdexcept>
#include "rapid_util.h"
#include <iostream>
#include <event2/thread.h>
#include <event2/buffer.h>
#include "defines.h"
#include <algorithm>

#if (RAPID_PLATFORM != RAPID_PLATFORM_WIN32)
#include <string.h>
#endif

namespace rapid
{
	RAPID_THREAD_DECLARATION(client_libevent_thread);

	r_bool network_client_impl::connect(const char* host, r_uint16 port)
	{
		if (m_connect_status != status_none)
		{
			return false;
		}

		if (!_is_activated())
		{
			m_end_libevent_thread = false;
			if (create_thread(client_libevent_thread, this) != 0)
			{
				m_end_libevent_thread = true;
#ifdef _DEBUG
				std::cout << "[network_engine]: create network thread failed!!!" << std::endl;
#endif
				return false;
			}
			for (; !m_libevent_thread_activated; )
			{
				r_sleep(0.001);
			}
		}

		_send_connect_command(host, port);

		return true;
	}
	void network_client_impl::_send_connect_command(const char* host, r_uint16 port)
	{
		m_connect_status = status_connecting;
		auto pCommand = m_buffered_commands.WriteLock();
		pCommand->m_command_id = buffered_command::BCS_CONNECT;
		pCommand->m_system_address.set(network_util::from_host(host), port);
		m_buffered_commands.WriteUnlock();
	}


	void network_client_impl::disconnect(r_bool wait /*= true*/, r_bool notify /*= false*/)
	{
		// todo
		if (_is_activated())
		{
			auto pCommand = m_buffered_commands.WriteLock();
			pCommand->m_command_id = buffered_command::BCS_DISCONNECT;
			pCommand->m_callback = notify;
			m_buffered_commands.WriteUnlock();
		}
	}

	void network_client_impl::set_timeout(r_uint32 time)
	{
		// todo
		//throw std::logic_error("The method or operation is not implemented.");
	}

	r_bool network_client_impl::is_connected() const
	{
		return m_connect_status == status_connected;
	}

	void network_client_impl::handle_message()
	{
		sock_packet** packet = nullptr;

		do
		{
			packet = m_incoming_packets.ReadLock();
			if (packet != NULL)
			{
				m_incoming_packets.ReadUnlock();
			}
			else
				break;

			auto& pPacket = **packet;

			switch (pPacket.m_type)
			{
			case sock_packet::packet_connected:
				_notify_server_connected(pPacket); break;
			case sock_packet::packet_disconnected:
				_notify_server_disconnected(pPacket); break;
			case sock_packet::packet_message:
				_notify_server_message(pPacket); break;
			case sock_packet::packet_connect_server_failed:
				_notify_connect_server_fail(pPacket);
				break;
			default: break;
			}

			_deallocate_packet(&pPacket);

		} while (true);
	}

	void network_client_impl::_notify_server_connected(sock_packet& pPacket)
	{
		r_assert(m_connect_status == status_connecting);
		m_connect_status = status_connected;
		m_host = pPacket.mSystemAddress;
		for (network_client_listeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
		{
			(*iter)->server_connected();
		}
	}

	void network_client_impl::_notify_server_full(sock_packet& pPacket)
	{
		r_assert(m_connect_status == status_connecting);
		m_connect_status = status_none;
		m_host = pPacket.mSystemAddress;
		for (network_client_listeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
		{
			(*iter)->server_full();
		}
	}

	void network_client_impl::_notify_connect_server_fail(sock_packet& pPacket)
	{
		r_assert(m_connect_status == status_connecting);
		m_connect_status = status_none;
		m_host = pPacket.mSystemAddress;
		for (network_client_listeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
		{
			(*iter)->connect_Failed();
		}
	}

	void network_client_impl::_notify_server_disconnected(sock_packet& pPacket)
	{
		r_assert(m_connect_status == status_connected);
		m_connect_status = status_none;
		for (network_client_listeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
		{
			(*iter)->server_disconnected();
		}
	}

	void network_client_impl::_notify_server_lost(sock_packet& pPacket)
	{
		r_assert(m_connect_status == status_connected);
		m_connect_status = status_none;
		for (network_client_listeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
		{
			(*iter)->server_lost();
		}
	}

	void network_client_impl::_notify_server_message(sock_packet& pPacket)
	{
		if (m_connect_status == status_connected)
		{
			for (network_client_listeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
			{
				(*iter)->server_message(pPacket.mData, pPacket.mLength);
			}
		}
	}

	void network_client_impl::add_listener(network_client_listener & listener)
	{
		network_client_listeners::iterator iter = std::find(m_listeners.begin(), m_listeners.end(), &listener);
		if (iter == m_listeners.end())
		{
			m_listeners.push_back(&listener);
		}
	}

	void network_client_impl::remove_listener(network_client_listener & listener)
	{
		network_client_listeners::iterator iter = std::find(m_listeners.begin(), m_listeners.end(), &listener);
		if (iter != m_listeners.end())
		{
			m_listeners.erase(iter);
		}
	}

	void network_client_impl::network_thread()
	{
#if (RAPID_PLATFORM == RAPID_PLATFORM_WIN32)
		evthread_use_windows_threads();
		int configflag = EVENT_BASE_FLAG_STARTUP_IOCP;
#else
		evthread_use_pthreads();
		int configflag = EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST;
#endif

		auto event_conf = event_config_new();
		event_config_set_flag(event_conf, configflag);
		event_config_set_num_cpus_hint(event_conf, get_cpus());
		m_event_base = event_base_new_with_config(event_conf);
		event_config_free(event_conf);

		m_libevent_thread_activated = true;

		for (; !m_end_libevent_thread; )
		{
			event_base_loopexit(m_event_base, &m_timev);
			event_base_dispatch(m_event_base);
			_process_outgoing_packets();
			_process_buffered_commands();
		}

		FREE_BUFFEREVENT(m_connection_tcp);
		FREE_EVENT_BASE(m_event_base);

		m_libevent_thread_activated = false;

	}

	void network_client_impl::_process_outgoing_packets(void)
	{
		for (auto pPacket = m_outgoing_packets.ReadLock(); pPacket != NULL; pPacket = m_outgoing_packets.ReadLock())
		{
			if (m_connection_tcp != NULL)
			{
				bufferevent_write(m_connection_tcp, (*pPacket)->mData, (*pPacket)->mLength);
			}
			_deallocate_packet(*pPacket);
			m_outgoing_packets.ReadUnlock();
		}
	}

	void network_client_impl::_process_buffered_commands(void)
	{
		for (auto pCommand = m_buffered_commands.ReadLock(); pCommand != NULL; pCommand = m_buffered_commands.ReadLock())
		{
			if (pCommand->m_command_id == buffered_command::BCS_CONNECT)
			{
				if (m_connection_tcp == NULL)
				{
					m_connection_tcp = bufferevent_socket_new(m_event_base, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
					r_assert(m_connection_tcp != NULL && "[network_engine]: bufferevent_socket_new failed!!!");

					bufferevent_setwatermark(m_connection_tcp, EV_READ | EV_WRITE, 0, 0);
					bufferevent_setcb(m_connection_tcp, network_client_impl::BUFFEREVENT_READ_CALLBACK, network_client_impl::BUFFEREVENT_WRITE_CALLBACK, network_client_impl::BUFFEREVENT_EVENT_CALLBACK, this);
					bufferevent_enable(m_connection_tcp, EV_READ | EV_WRITE | EV_TIMEOUT);


					struct sockaddr_in sin; memset(&sin, 0, sizeof(struct sockaddr_in));
					sin.sin_family = AF_INET;
					sin.sin_addr.s_addr = pCommand->m_system_address.get_binary_address();
					sin.sin_port = htons(pCommand->m_system_address.get_port());
					if (bufferevent_socket_connect(m_connection_tcp, (struct sockaddr*)&sin, sizeof(struct sockaddr_in)) != 0)
					{
#ifdef _DEBUG
						std::cout << "[network_engine]: bufferevent_socket_connect failed!!!" << std::endl;
#endif
						FREE_BUFFEREVENT(m_connection_tcp);
						_push_packet_connection_failed(true);
					}
					else
					{

						evutil_socket_t fd = bufferevent_getfd(m_connection_tcp);
						int option = 1; int len = sizeof(long);
						int r = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&option, len);
#ifdef _DEBUG
						std::cout << "[network_engine]: bufferevent_socket_connect process, wait for accept!!!" << std::endl;
#endif
					}
				}
				//else
				//{
				//	_push_packet_already_connected(true);
				//}
			}
			else if (pCommand->m_command_id == buffered_command::BCS_DISCONNECT)
			{
				FREE_BUFFEREVENT(m_connection_tcp);
				_push_packet_connection_breaked(pCommand->m_callback);
			}
			m_buffered_commands.ReadUnlock();
		}
	}

	sock_packet* network_client_impl::_allocate_packet(r_uint32 nLength)
	{
		auto pPacket = (sock_packet*)malloc(sizeof(sock_packet) + nLength);
		pPacket->mSystemAddress.reset();
		pPacket->mData = (unsigned char*)pPacket + sizeof(sock_packet);
		pPacket->mLength = nLength;
		return pPacket;
	}

	void network_client_impl::_deallocate_packet(sock_packet* pPacket)
	{
		free(pPacket);
	}

	void network_client_impl::_process_receive(struct bufferevent* pConnectionTCP)
	{
		r_assert(m_connection_tcp == pConnectionTCP);
		struct evbuffer* evinput = bufferevent_get_input(m_connection_tcp);
		r_uint32 nInputLength = evbuffer_get_length(evinput);

		const r_uint32 need_len = sizeof(PACKET_LEN_TYPE) + PACKET_HEADER_SIZE;

		while (nInputLength >= need_len)
		{
			unsigned char PACKET_HEADER[need_len] = { 0 };
			evbuffer_copyout(evinput, PACKET_HEADER, need_len);

#ifdef USE_PACKET_HEADER
			if ((*(r_uint32*)PACKET_HEADER) != INNER_PACKET_HEADER)
			{
				_connection_breaked(pIncomingConnection, true);
				FREE_BUFFEREVENT(pConnectionTCP);
				break;
			}
#endif
			PACKET_LEN_TYPE packet_len = (*(PACKET_LEN_TYPE*)(PACKET_HEADER + PACKET_HEADER_SIZE));
			if (packet_len <= nInputLength - need_len)
			{
				sock_packet** pPacket = m_incoming_packets.WriteLock();
				(*pPacket) = _allocate_packet(packet_len);
				evbuffer_remove(evinput, PACKET_HEADER, need_len);
				evbuffer_remove(evinput, (*pPacket)->mData, packet_len);
				(*pPacket)->m_type = sock_packet::packet_message;
				m_incoming_packets.WriteUnlock();
				nInputLength = evbuffer_get_length(evinput);
				continue;
			}
			else
				break;
		}
	}

	void network_client_impl::_process_event(struct bufferevent* pConnectionTCP, short nEventFlag)
	{
		r_assert(m_connection_tcp == pConnectionTCP);
		if ((nEventFlag & BEV_EVENT_CONNECTED) != 0)
		{
#ifdef _DEBUG
			std::cout << "[network_engine]: network_client_impl::BEV_EVENT_CONNECTED" << std::endl;
#endif
			//unsigned char SEND_PACKET[sizeof(r_uint32) * 4] = { 0 };
			//(*(r_uint32*)(SEND_PACKET)) = INNER_PACKET_HEADER;
			//(*(r_uint32*)(SEND_PACKET + sizeof(r_uint32))) = sizeof(r_uint32) * 2;
			//(*(r_uint32*)(SEND_PACKET + sizeof(r_uint32) * 2)) = INNER_AUTHORIZE_ID;
			//(*(r_uint32*)(SEND_PACKET + sizeof(r_uint32) * 3)) = INNER_CONNECT_PASSWORD;
			//struct timeval timev;
			//timev.tv_sec = 0; timev.tv_usec = 3000 * 1000;
			//bufferevent_set_timeouts(mConnectionTCP, &timev, &timev);
			//bufferevent_write(m_connection_tcp, SEND_PACKET, sizeof(r_uint32) * 4);
			_push_packet_connection_accepted(true);
		}
		else if (((nEventFlag & BEV_EVENT_TIMEOUT) != 0))
		{
#ifdef _DEBUG
			std::cout << "[network_engine]: network_client_impl::BEV_EVENT_TIMEOUT" << std::endl;
#endif
			FREE_BUFFEREVENT(m_connection_tcp);
			_push_packet_connection_failed(true);
		}
		else if (((nEventFlag & BEV_EVENT_EOF) != 0))
		{
#ifdef _DEBUG
			std::cout << "[network_engine]: network_client_impl::BEV_EVENT_EOF" << std::endl;
#endif
			FREE_BUFFEREVENT(m_connection_tcp);
			_push_packet_connection_failed(true);
		}
		else if (((nEventFlag & BEV_EVENT_ERROR) != 0))
		{
#ifdef _DEBUG
			std::cout << "[network_engine]: network_client_impl::BEV_EVENT_ERROR" << std::endl;
#endif
			FREE_BUFFEREVENT(m_connection_tcp);
			if (m_connect_status == status_connected)
				_push_packet_connection_breaked(true);
			else
				_push_packet_connection_failed(true);
		}
	}

	void network_client_impl::_push_packet_connection_failed(bool callback)
	{
#ifdef _DEBUG
		std::cout << "[network_engine]: " << __FUNCTION__ << std::endl;
#endif
		if (callback)
		{
			sock_packet** pPacket = m_incoming_packets.WriteLock();
			(*pPacket) = _allocate_packet(0);
			(*pPacket)->m_type = sock_packet::packet_connect_server_failed;
			(*pPacket)->mData = nullptr;
			m_incoming_packets.WriteUnlock();
		}
	}

	void network_client_impl::_push_packet_connection_accepted(bool callback)
	{
#ifdef _DEBUG
		std::cout << "[network_engine]: " << __FUNCTION__ << std::endl;
#endif
		struct sockaddr_in sin; memset(&sin, 0, sizeof(struct sockaddr_in));
		evutil_socket_t fd = bufferevent_getfd(m_connection_tcp);
		int socklen = sizeof(struct sockaddr_in);
		//m_internal_id.reset();
		//m_external_id.reset();
		//if (getsockname(fd, (struct sockaddr*)&sin, &socklen) == 0)
		//{
		//	m_internal_id.set(sin.sin_addr.s_addr, ntohs(sin.sin_port));
		//}
		//if (getpeername(fd, (struct sockaddr*)&sin, &socklen) == 0)
		//{
		//	m_external_id.set(sin.sin_addr.s_addr, ntohs(sin.sin_port));
		//}

		if (callback)
		{
			sock_packet** pPacket = m_incoming_packets.WriteLock();
			(*pPacket) = _allocate_packet(0);
			(*pPacket)->m_type = sock_packet::packet_connected;
			(*pPacket)->mData = nullptr;
			m_incoming_packets.WriteUnlock();
		}
	}

	void network_client_impl::_push_packet_connection_breaked(bool callback)
	{
#ifdef _DEBUG
		std::cout << "[network_engine]: " << __FUNCTION__ << std::endl;
#endif
		for (sock_packet** pPacket = m_outgoing_packets.ReadLock(); pPacket != NULL; pPacket = m_outgoing_packets.ReadLock())
		{
			_deallocate_packet(*pPacket);
			m_outgoing_packets.ReadUnlock();
		}
		if (callback)
		{
			sock_packet** pPacket = m_incoming_packets.WriteLock();
			(*pPacket) = _allocate_packet(0);
			(*pPacket)->m_type = sock_packet::packet_disconnected;
			(*pPacket)->mData = nullptr;
			m_incoming_packets.WriteUnlock();
		}
	}

	//void network_client_impl::_push_packet_already_connected(bool callback)
	//{
	//	std::cout << "[network_engine]: network_client_impl::_push_packet_already_connected" << std::endl;
	//	if (callback)
	//	{
	//		sock_packet** pPacket = mIncomingPackets.WriteLock();
	//		(*pPacket) = AllocatePacket(1);
	//		(*pPacket)->mData[0] = packet_type::type_connect_already;
	//		mIncomingPackets.WriteUnlock();
	//	}
	//}

	void network_client_impl::BUFFEREVENT_READ_CALLBACK(struct bufferevent* pConnectionTCP, void* pArgs)
	{
		static_cast<network_client_impl*>(pArgs)->_process_receive(pConnectionTCP);
	}

	void network_client_impl::BUFFEREVENT_WRITE_CALLBACK(struct bufferevent* pConnectionTCP, void* pArgs)
	{
	}

	void network_client_impl::BUFFEREVENT_EVENT_CALLBACK(struct bufferevent* pConnectionTCP, short nEventFlag, void* pArgs)
	{
		static_cast<network_client_impl*>(pArgs)->_process_event(pConnectionTCP, nEventFlag);
	}

	void network_client_impl::send(void* data, r_uint32 len)
	{
		if (_is_activated())
		{
			r_uint32 pos = 0;
			r_uint32 header_size = PACKET_HEADER_SIZE;
			r_uint32 packet_len_size = sizeof(PACKET_LEN_TYPE);

#ifdef USE_PACKET_HEADER
			static const r_uint32 PACKET_HEADER = INNER_PACKET_HEADER;
#endif

			auto pPacket = m_outgoing_packets.WriteLock();
			(*pPacket) = _allocate_packet(len + header_size + packet_len_size);

#ifdef USE_PACKET_HEADER
			memcpy((*pPacket)->mData, &PACKET_HEADER, sizeof(PACKET_HEADER));
			pos += sizeof(PACKET_HEADER);
#endif
			// len
			memcpy((*pPacket)->mData + pos, &len, packet_len_size);
			pos += packet_len_size;

			//data
			memcpy((*pPacket)->mData + pos, data, len);

			m_outgoing_packets.WriteUnlock();
		}
	}

	RAPID_THREAD_DECLARATION(client_libevent_thread)
	{
		static_cast<network_client_impl*>(args)->network_thread();
		return 0;
	}

}