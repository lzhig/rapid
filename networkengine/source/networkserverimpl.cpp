#include "networkserverimpl.h"
#include <stdexcept>

#include <event2/event_compat.h>
#include <event2/buffer.h>
#include <event2/thread.h>

#include <iostream>
#include "rapid_util.h"
#include "rapid.h"
#include "defines.h"
#include <algorithm>

#if (RAPID_PLATFORM != RAPID_PLATFORM_WIN32)
#include <string.h>
#endif

namespace rapid
{
	RAPID_THREAD_DECLARATION(server_libevent_thread);

	network_server_impl::network_server_impl()
	{

	}

	network_server_impl::~network_server_impl()
	{
		FREE_EVCONNLISTENER(m_conn_listener);
		FREE_EVENT_BASE(m_event_base);
	}

	r_bool network_server_impl::start(const char* host, r_uint16 port, r_uint32 max_connections)
	{
		if (_is_activated() || max_connections <= 0)
			return false;

		m_max_connections = max_connections;
		m_client_connections.resize(m_max_connections);

		r_uint32 i = 0;
		for_each(m_client_connections.begin(), m_client_connections.end(), [&i](client_connection& conn) {conn.m_connection_id = i++; });

		m_host_address.set(
			host == nullptr ? 0 : network_util::from_host(host),
			port);

		m_end_libevent_thread = false;
		if (create_thread(server_libevent_thread, this) != 0)
		{
			m_end_libevent_thread = true;
#ifdef _DEBUG
			std::cout << "[network_engine]: create network thread failed!!!" << std::endl;
#endif
			return false;
		}

		for (; !m_libevent_thread_activated && !m_end_libevent_thread;)
		{
			r_sleep(0.01);
		}

		return true;
	}

	void network_server_impl::stop(r_uint32 wait_time /*= 0*/)
	{
		m_end_libevent_thread = true;
		for (; m_libevent_thread_activated; )
		{
			r_sleep(0.001);
		}
		for (auto pPacket = m_incoming_packets.ReadLock(); pPacket != NULL; pPacket = m_incoming_packets.ReadLock())
		{
			_deallocate_packet(*pPacket);
			m_incoming_packets.ReadUnlock();
		}
		for (auto pPacket = m_outgoing_packets.ReadLock(); pPacket != NULL; pPacket = m_outgoing_packets.ReadLock())
		{
			_deallocate_packet(*pPacket);
			m_outgoing_packets.ReadUnlock();
		}
	}

	void network_server_impl::disconnect(void * net, bool notify)
	{
		auto conn = static_cast<client_connection*>(net);

		auto pCommand = m_buffered_commands.WriteLock();
		pCommand->m_command_id = buffered_command::BCS_DISCONNECT;
		pCommand->m_system_address = conn->m_client_address;
		pCommand->m_callback = notify;
		m_buffered_commands.WriteUnlock();
	}

	void network_server_impl::disconnect_all_clients(r_bool notify /*= true*/)
	{
		for (r_uint32 i = 0; i < m_max_connections; ++i)
		{
			if (m_client_connections[i].m_active)
			{
				disconnect(&m_client_connections[i]);
			}
		}
	}

	void network_server_impl::set_timeout(r_uint32 time)
	{
		// todo
		//throw std::logic_error("The method or operation is not implemented.");
	}

	r_bool network_server_impl::is_running() const
	{
		return !m_end_libevent_thread && m_libevent_thread_activated;
	}

	void network_server_impl::handle_message()
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
				_clientConnected(pPacket); break;
			case sock_packet::packet_disconnected:
				_clientDisconnected(pPacket); break;
			case sock_packet::packet_message:
				_clientMessage(pPacket); break;
			default: r_assert(false);
			}

			_deallocate_packet(&pPacket);

		} while (true);
	}

	r_bool network_server_impl::_is_activated() const
	{
		return !m_end_libevent_thread && m_libevent_thread_activated;
	}

	void network_server_impl::network_thread()
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


		struct sockaddr_in sin; 
		memset(&sin, 0, sizeof(struct sockaddr_in));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = m_host_address.get_binary_address();
		sin.sin_port = htons(m_host_address.get_port());
		m_conn_listener = evconnlistener_new_bind(m_event_base, network_server_impl::EVCONNLISTENER_CALLBACK, this,
			LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE | LEV_OPT_THREADSAFE, -1, (struct sockaddr*)&sin, sizeof(struct sockaddr_in));

		if (m_conn_listener != nullptr)
		{
			//memset(&sin, 0, sizeof(struct sockaddr_in)); 
			//int socklen = sizeof(struct sockaddr_in);
			//evutil_socket_t fd = evconnlistener_get_fd(m_conn_listener);

			//if (getsockname(fd, (struct sockaddr*)&sin, &socklen) == 0)
			//{
			//	mInternalID.mBinaryAddress = sin.sin_addr.s_addr;
			//	mInternalID.mPort = ntohs(sin.sin_port);
			//}
		}
		else
		{
#ifdef _DEBUG
			std::cout << "[network_engine]: evconnlistener_new_bind failed!!!" << std::endl;
#endif
			m_end_libevent_thread = true;
			return;
		}

		m_free_connections.resize(m_max_connections);
		for (r_uint32 i = 0; i < m_max_connections; ++i)
		{
			m_free_connections[i] = &m_client_connections[i];
		}

		m_libevent_thread_activated = true;

		for (; !m_end_libevent_thread; )
		{
			event_base_loopexit(m_event_base, &m_timev);
			event_base_dispatch(m_event_base);
			_process_outgoing_packets();
			_process_buffered_commands();
		}

		FREE_EVCONNLISTENER(m_conn_listener);
		FREE_EVENT_BASE(m_event_base);

		m_libevent_thread_activated = false;

	}

	void network_server_impl::EVCONNLISTENER_CALLBACK(evconnlistener * pListenerTCP, evutil_socket_t fd, sockaddr * sin, int socklen, void * pArgs)
	{
		static_cast<network_server_impl*>(pArgs)->_process_accept(pListenerTCP, fd, sin, socklen);
	}

	void network_server_impl::BUFFEREVENT_READ_CALLBACK(bufferevent * pConnectionTCP, void * pArgs)
	{
		static_cast<network_server_impl*>(pArgs)->_process_receive(pConnectionTCP);
	}

	void network_server_impl::BUFFEREVENT_WRITE_CALLBACK(bufferevent * pConnectionTCP, void * pArgs)
	{
		// todo
	}

	void network_server_impl::BUFFEREVENT_EVENT_CALLBACK(bufferevent * pConnectionTCP, short nEventFlag, void * pArgs)
	{
		static_cast<network_server_impl*>(pArgs)->_process_event(pConnectionTCP, nEventFlag);
	}

	void network_server_impl::_process_outgoing_packets(void)
	{
		for (auto pPacket = m_outgoing_packets.ReadLock(); pPacket != NULL; pPacket = m_outgoing_packets.ReadLock())
		{
			auto connectionIter = mAddressedConnections.find((*pPacket)->mSystemAddress);
			if (connectionIter != mAddressedConnections.end())
			{
				auto pIncomingConnection = connectionIter->second;
				if (pIncomingConnection->m_connection_tcp != NULL)
				{
					bufferevent_write(pIncomingConnection->m_connection_tcp, (*pPacket)->mData, (*pPacket)->mLength);
				}
			}
			_deallocate_packet(*pPacket);
			m_outgoing_packets.ReadUnlock();
		}
	}

	void network_server_impl::_process_buffered_commands(void)
	{
		for (auto pCommand = m_buffered_commands.ReadLock(); pCommand != NULL; pCommand = m_buffered_commands.ReadLock())
		{
			if (pCommand->m_command_id == buffered_command::BCS_DISCONNECT)
			{
				auto connectionIter = mAddressedConnections.find(pCommand->m_system_address);
				if (connectionIter != mAddressedConnections.end())
				{
					auto pIncomingConnection = connectionIter->second;
					auto pConnectionTCP = pIncomingConnection->m_connection_tcp;
					_connection_breaked(connectionIter->second, pCommand->m_callback);
					bufferevent_free(pConnectionTCP);
				}
			}
			m_buffered_commands.ReadUnlock();
		}
	}

	void network_server_impl::_connection_accepted(client_connection * pIncomingConnection)
	{
#ifdef _DEBUG
		std::cout << "[network_engine]: " << __FUNCTION__ << std::endl;
#endif
		auto pPacket = m_incoming_packets.WriteLock();
		(*pPacket) = _allocate_packet(0);
		(*pPacket)->m_type = sock_packet::packet_connected;
		(*pPacket)->mData = nullptr;
		(*pPacket)->mIndex = pIncomingConnection->m_connection_id;
		(*pPacket)->mSystemAddress = pIncomingConnection->m_client_address;
		m_incoming_packets.WriteUnlock();
	}

	void network_server_impl::_connection_breaked(client_connection * pIncomingConnection, bool callback)
	{
#ifdef _DEBUG
		std::cout << "[network_engine]: " << __FUNCTION__ << std::endl;
#endif

		auto connectionIter = mIncomingConnections.find(pIncomingConnection->m_connection_tcp);
		r_assert(connectionIter != mIncomingConnections.end());
		mIncomingConnections.erase(connectionIter);

		auto addressIter = mAddressedConnections.find(pIncomingConnection->m_client_address);
		r_assert(addressIter != mAddressedConnections.end());
		mAddressedConnections.erase(addressIter);

		pIncomingConnection->m_connection_tcp = NULL;
		pIncomingConnection->m_client_address.reset();
		m_free_connections.push_back(pIncomingConnection);
		if (callback)
		{
			auto pPacket = m_incoming_packets.WriteLock();
			(*pPacket) = _allocate_packet(0);
			(*pPacket)->m_type = sock_packet::packet_disconnected;
			(*pPacket)->mData = nullptr;
			(*pPacket)->mIndex = pIncomingConnection->m_connection_id;
			(*pPacket)->mSystemAddress = pIncomingConnection->m_client_address;
			m_incoming_packets.WriteUnlock();
		}
	}

	void network_server_impl::_connection_failed(client_connection * pIncomingConnection)
	{
#ifdef _DEBUG
		std::cout << "[network_engine]: " << __FUNCTION__ << std::endl;
#endif
		kIncomingConnections_t::iterator connectionIter = mIncomingConnections.find(pIncomingConnection->m_connection_tcp);
		r_assert(connectionIter != mIncomingConnections.end());
		mIncomingConnections.erase(connectionIter);
		auto addressIter = mAddressedConnections.find(pIncomingConnection->m_client_address);
		r_assert(addressIter != mAddressedConnections.end());
		mAddressedConnections.erase(addressIter);
		pIncomingConnection->m_connection_tcp = NULL;
		pIncomingConnection->m_client_address.reset();
		m_free_connections.push_back(pIncomingConnection);
	}

	void network_server_impl::_process_accept(evconnlistener * pListenerTCP, evutil_socket_t fd, sockaddr * sin, int socklen)
	{
		if (!m_free_connections.empty())
		{
			auto pIncomingConnection = m_free_connections[m_free_connections.size() - 1];
			auto pConnectionTCP = bufferevent_socket_new(m_event_base, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
			int option = 1;
			int len = sizeof(int);
			int r = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&option, len);
			r_assert(pConnectionTCP != NULL && "[network_engine]: bufferevent_socket_new failed!!!");

			bufferevent_setwatermark(pConnectionTCP, EV_READ | EV_WRITE, 0, 0);
			bufferevent_setcb(pConnectionTCP, network_server_impl::BUFFEREVENT_READ_CALLBACK, network_server_impl::BUFFEREVENT_WRITE_CALLBACK, network_server_impl::BUFFEREVENT_EVENT_CALLBACK, this);
			bufferevent_enable(pConnectionTCP, EV_READ | EV_WRITE | EV_TIMEOUT);
			
			//struct timeval timev;
			//timev.tv_sec = 0; timev.tv_usec = 3000 * 1000;
			//bufferevent_set_timeouts(pConnectionTCP, &timev, &timev);
			
			pIncomingConnection->m_client_address.set(((struct sockaddr_in*)sin)->sin_addr.s_addr, ((struct sockaddr_in*)sin)->sin_port);
			pIncomingConnection->m_connection_tcp = pConnectionTCP;
			mIncomingConnections[pConnectionTCP] = pIncomingConnection;
			mAddressedConnections[pIncomingConnection->m_client_address] = pIncomingConnection;
			m_free_connections.pop_back();

			_connection_accepted(pIncomingConnection);
#ifdef _DEBUG
			std::cout << "[network_engine]: new incoming connection accepted, do authorize!!!" << std::endl;
#endif
		}
		else
		{
#ifdef _DEBUG
			std::cout << "[network_engine]: no free systems, disconnect new incoming connection!!!" << std::endl;
#endif
			evutil_closesocket(fd);
		}
	}

	void network_server_impl::_process_receive(bufferevent * pConnectionTCP)
	{
		kIncomingConnections_t::iterator connectionIter = mIncomingConnections.find(pConnectionTCP);
		r_assert(connectionIter != mIncomingConnections.end());

		auto pIncomingConnection = connectionIter->second;
		r_assert(pIncomingConnection->m_connection_tcp == pConnectionTCP);

		struct evbuffer* evinput = bufferevent_get_input(pConnectionTCP);
		r_uint32 nInputLength = evbuffer_get_length(evinput);

		const r_uint32 need_len = sizeof(PACKET_LEN_TYPE) + PACKET_HEADER_SIZE;

		while (nInputLength >= need_len)
		{
			unsigned char PACKET_HEADER[need_len] = { 0 };
			evbuffer_copyout(evinput, PACKET_HEADER, need_len);

#ifdef USE_PACKET_HEADER
			if (
				(*(r_uint32*)PACKET_HEADER) != INNER_PACKET_HEADER
				)
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
				(*pPacket)->mSystemAddress = pIncomingConnection->m_client_address;
				(*pPacket)->mIndex = pIncomingConnection->m_connection_id;
				m_incoming_packets.WriteUnlock();
				nInputLength = evbuffer_get_length(evinput);
				continue;
			}
			else
				break;
		}
	}

	void network_server_impl::_process_event(bufferevent * pConnectionTCP, short nEventFlag)
	{
		kIncomingConnections_t::iterator connectionIter = mIncomingConnections.find(pConnectionTCP);
		r_assert(connectionIter != mIncomingConnections.end());
		auto pIncomingConnection = connectionIter->second;
		r_assert(pIncomingConnection->m_connection_tcp == pConnectionTCP);
		if (((nEventFlag & BEV_EVENT_TIMEOUT) != 0))
		{
#ifdef _DEBUG
			std::cout << "[network_engine]: kLibeventCLient::BEV_EVENT_TIMEOUT" << std::endl;
#endif
			_connection_failed(pIncomingConnection);
			FREE_BUFFEREVENT(pConnectionTCP);
		}
		else if (((nEventFlag & BEV_EVENT_EOF) != 0))
		{
#ifdef _DEBUG
			std::cout << "[network_engine]: kLibeventCLient::BEV_EVENT_EOF" << std::endl;
#endif
			_connection_breaked(pIncomingConnection, true);
			FREE_BUFFEREVENT(pConnectionTCP);
		}
		else if (((nEventFlag & BEV_EVENT_ERROR) != 0))
		{
#ifdef _DEBUG
			std::cout << "[network_engine]: kLibeventCLient::BEV_EVENT_ERROR" << std::endl;
#endif
			_connection_breaked(pIncomingConnection, true);
			FREE_BUFFEREVENT(pConnectionTCP);
		}
	}

	void network_server_impl::_clientConnected(sock_packet & pPacket)
	{
		client_connection& conn = m_client_connections[pPacket.mIndex];
		conn.m_client_address = pPacket.mSystemAddress;
		conn.m_active = true;
		conn.m_bind_data = nullptr;
		for (kListeners_t::iterator e = mListeners.begin(); e != mListeners.end(); ++e)
		{
			(*e)->client_connected(&m_client_connections[pPacket.mIndex]);
		}
	}

	void network_server_impl::_clientDisconnected(sock_packet & pPacket)
	{
		for (kListeners_t::iterator e = mListeners.begin(); e != mListeners.end(); ++e)
		{
			(*e)->client_disconnected(&m_client_connections[pPacket.mIndex]);
		}

		client_connection& conn = m_client_connections[pPacket.mIndex];
		conn.m_active = false;
	}

	void network_server_impl::_clientLost(sock_packet & pPacket)
	{
		for (kListeners_t::iterator e = mListeners.begin(); e != mListeners.end(); ++e)
		{
			(*e)->client_lost(&m_client_connections[pPacket.mIndex]);
		}
	}

	void network_server_impl::_clientMessage(sock_packet & pPacket)
	{
		for (kListeners_t::iterator e = mListeners.begin(); e != mListeners.end(); ++e)
		{
			(*e)->client_message(&m_client_connections[pPacket.mIndex], pPacket.mData, pPacket.mLength);
		}
	}

	sock_packet* network_server_impl::_allocate_packet(r_uint32 nLength)
	{
		sock_packet* pPacket = (sock_packet*)malloc(sizeof(sock_packet) + nLength);
		pPacket->mSystemAddress.reset();
		pPacket->mData = (unsigned char*)pPacket + sizeof(sock_packet);
		pPacket->mLength = nLength;
		return pPacket;
	}

	void network_server_impl::_deallocate_packet(sock_packet* packet)
	{
		free(packet);
	}

	void network_server_impl::set_binding_data(void * net, void * data)
	{
		auto conn = static_cast<client_connection*>(net);
		conn->m_bind_data = data;
	}

	void* network_server_impl::get_binding_data(void * net)
	{
		auto conn = static_cast<client_connection*>(net);
		return conn->m_bind_data;
	}

	void network_server_impl::add_listener(network_server_listener & listener)
	{
		kListeners_t::iterator iter = std::find(mListeners.begin(), mListeners.end(), &listener);
		if (iter == mListeners.end())
		{
			mListeners.push_back(&listener);
		}
	}

	void network_server_impl::remove_listener(network_server_listener & listener)
	{
		kListeners_t::iterator iter = std::find(mListeners.begin(), mListeners.end(), &listener);
		if (iter != mListeners.end())
		{
			mListeners.erase(iter);
		}
	}

	void network_server_impl::send(void* net, void* data, r_uint32 len)
	{
		const client_connection& con = *static_cast<client_connection*>(net);
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

			(*pPacket)->mSystemAddress = con.m_client_address;

			m_outgoing_packets.WriteUnlock();
		}

	}


	RAPID_THREAD_DECLARATION(server_libevent_thread)
	{
		static_cast<network_server_impl*>(args)->network_thread();
		return 0;
	}


}