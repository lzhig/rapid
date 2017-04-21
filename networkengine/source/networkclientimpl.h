#pragma once

#include "../include/networkengine.h"
#include <event2/event.h>
#include "single_producer_consumer.h"
#include "networkserverimpl.h"

using namespace rapidlib;

namespace rapid
{
	class network_client_impl : public network_client
	{
	public:

		// Í¨¹ý network_client ¼Ì³Ð
		virtual r_bool connect(const char* host, r_uint16 port) override;

		virtual void disconnect(r_bool wait = true, r_bool notify = false) override;

		virtual void set_timeout(r_uint32 time) override;

		virtual r_bool is_connected() const override;

		virtual void handle_message() override;

		virtual void add_listener(network_client_listener & listener) override;

		virtual void remove_listener(network_client_listener & listener) override;

		void network_thread();

		virtual void send(void* data, r_uint32 len) override;

	private:
		void _send_connect_command(const char* host, r_uint16 port);

		r_bool _is_activated() const { return !m_end_libevent_thread && m_libevent_thread_activated; }

		void _process_outgoing_packets(void);
		void _process_buffered_commands(void);

		sock_packet* _allocate_packet(r_uint32 nLength);
		void _deallocate_packet(sock_packet* pPacket);

		static void BUFFEREVENT_READ_CALLBACK(struct bufferevent* pConnectionTCP, void* pArgs);
		static void BUFFEREVENT_WRITE_CALLBACK(struct bufferevent* pConnectionTCP, void* pArgs);
		static void BUFFEREVENT_EVENT_CALLBACK(struct bufferevent* pConnectionTCP, short nEventFlag, void* pArgs);

		void _process_receive(struct bufferevent* pConnectionTCP);
		void _process_event(struct bufferevent* pConnectionTCP, short nEventFlag);

		void _push_packet_connection_failed(bool callback);
		void _push_packet_connection_accepted(bool callback);
		void _push_packet_connection_breaked(bool callback);
		//void _push_packet_already_connected(bool callback);

		void _notify_server_connected(sock_packet& pPacket);
		void _notify_server_full(sock_packet& pPacket);
		void _notify_connect_server_fail(sock_packet& pPacket);
		void _notify_server_disconnected(sock_packet& pPacket);
		void _notify_server_lost(sock_packet& pPacket);
		void _notify_server_message(sock_packet& pPacket);


		enum connect_status
		{
			status_none,
			status_connecting,
			status_connected,
			status_disconnecting,
			status_disconnected,
		};
		connect_status										m_connect_status = connect_status::status_none;

		volatile r_bool										m_authorised = false;
		volatile r_bool										m_end_libevent_thread = true;
		volatile r_bool										m_libevent_thread_activated = false;

		struct event_base*									m_event_base = nullptr;
		struct bufferevent*                                 m_connection_tcp = nullptr;

		single_producer_consumer<buffered_command>			m_buffered_commands;
		single_producer_consumer<sock_packet*>				m_incoming_packets;
		single_producer_consumer<sock_packet*>				m_outgoing_packets;

		//socket_address										m_internal_id;
		//socket_address										m_external_id;
		socket_address										m_host;

		typedef std::vector<network_client_listener*>		network_client_listeners;
		network_client_listeners							m_listeners;

		const struct timeval				m_timev = { 0, 1000 };

	};
}