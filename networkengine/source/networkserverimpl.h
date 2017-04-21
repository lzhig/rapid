#pragma once
#include "../include/networkengine.h"

#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>

#include "network_util.h"
#include <vector>
#include <map>

#include "single_producer_consumer.h"
#include "defines.h"

using namespace rapidlib;

namespace rapid
{

	class network_server_impl : public network_server
	{
	public:
		network_server_impl();
		~network_server_impl();

		// 通过 network_server 继承
		virtual r_bool start(const char* host, r_uint16 port, r_uint32 max_connections) override;

		virtual void stop(r_uint32 wait_time = 0) override;

		virtual void disconnect(void * net, bool notify = true) override;

		virtual void disconnect_all_clients(r_bool notify = true) override;

		virtual void set_timeout(r_uint32 time) override;

		virtual r_bool is_running() const override;

		virtual void handle_message() override;

		virtual void add_listener(network_server_listener & listener) override;

		virtual void remove_listener(network_server_listener & listener) override;

		virtual void set_binding_data(void * net, void * data) override;

		virtual void* get_binding_data(void * net) override;

		virtual void send(void* net, void* data, r_uint32 len) override;

		void network_thread();

	private:

		class client_connection
		{
		public:
			socket_address			m_client_address;					// 客户端连接地址信息
			mutable void*			m_bind_data = nullptr;				// 绑定数据
			r_uint32				m_connection_id = 0;				// 连接id
			r_bool					m_active = false;					// 连接是否激活


			struct bufferevent*		m_connection_tcp = nullptr;
		};


		r_bool _is_activated() const;


		static void EVCONNLISTENER_CALLBACK(struct evconnlistener* pListenerTCP, evutil_socket_t fd, struct sockaddr* sin, int socklen, void* pArgs);
		static void BUFFEREVENT_READ_CALLBACK(struct bufferevent* pConnectionTCP, void* pArgs);
		static void BUFFEREVENT_WRITE_CALLBACK(struct bufferevent* pConnectionTCP, void* pArgs);
		static void BUFFEREVENT_EVENT_CALLBACK(struct bufferevent* pConnectionTCP, short nEventFlag, void* pArgs);

		void _process_outgoing_packets(void);
		void _process_buffered_commands(void);

		void _connection_accepted(client_connection* pIncomingConnection);
		void _connection_breaked(client_connection* pIncomingConnection, bool callback);
		void _connection_failed(client_connection* pIncomingConnection);

		void _process_accept(struct evconnlistener* pListenerTCP, evutil_socket_t fd, struct sockaddr* sin, int socklen);
		void _process_receive(struct bufferevent* pConnectionTCP);
		void _process_event(struct bufferevent* pConnectionTCP, short nEventFlag);

		void _clientConnected(sock_packet& pPacket);
		void _clientDisconnected(sock_packet& pPacket);
		void _clientLost(sock_packet& pPacket);
		void _clientMessage(sock_packet& pPacket);



		sock_packet * _allocate_packet(r_uint32 nLength);
		void _deallocate_packet(sock_packet* packet);



		std::vector<client_connection>					m_client_connections;
		std::vector<client_connection*>					m_free_connections;

		r_uint32										m_max_connections = 0;	// 连接最大上限
		r_bool											m_need_auth = false;	// 连接时是否需要校验

		socket_address									m_host_address;

		volatile r_bool											m_end_libevent_thread = true;
		volatile r_bool											m_libevent_thread_activated = false;

		struct event_base*								m_event_base = nullptr;
		struct evconnlistener*							m_conn_listener = nullptr;


		single_producer_consumer<sock_packet*>					m_incoming_packets;
		single_producer_consumer<sock_packet*>					m_outgoing_packets;
		single_producer_consumer<buffered_command>				m_buffered_commands;


		typedef std::map<socket_address, client_connection*>    address_connections_map;
		address_connections_map                                 mAddressedConnections;


		typedef std::map<void*, client_connection*>				kIncomingConnections_t;
		kIncomingConnections_t                                  mIncomingConnections;


		typedef std::vector<network_server_listener*>			kListeners_t;
		kListeners_t											mListeners;

		const struct timeval				m_timev = { 0, 1000 };

	};
}