#pragma once

#include "network_util.h"

using namespace rapidlib;

//enum packet_type
//{
//	type_connect,
//	type_connect_attempt_failed,
//	type_connect_accepted,
//	type_connect_already,
//	type_disconnect,
//	type_message,
//};

#ifdef USE_PACKET_HEADER
#define INNER_PACKET_HEADER                     (0xFEDCBA98)    /**< inner packet header. */
#define PACKET_HEADER_SIZE						(sizeof(INNER_PACKET_HEADER))	// bytes of packet header
#else
#define PACKET_HEADER_SIZE						(0)	// bytes of packet header
#endif

typedef r_uint16	PACKET_LEN_TYPE;

#define INNER_CONNECT_PASSWORD                  (0x12345678)    /**< connection password. */

#define INNER_AUTHORIZE_ID                      (0)
#define INNER_AUTHORIZE_ACK_ID                  (1)


#define FREE_EVENT_BASE(p)                  {if(p) {event_base_free(p); p = NULL;}}
#define FREE_EVENT_CONFIG(p)                {if(p) {event_config_free(p); p = NULL;}}
#define FREE_EVCONNLISTENER(p)              {if(p) {evconnlistener_free(p); p = NULL;}}
#define FREE_BUFFEREVENT(p)                 {if(p) {bufferevent_free(p); p = NULL;}}

typedef struct buffered_command {
	enum {
		BCS_CONNECT,
		BCS_DISCONNECT
	}													m_command_id;
	socket_address										m_system_address;
	r_bool												m_callback;
} buffered_command;
