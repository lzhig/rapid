#include "networkengineimpl.h"
#include "networkclientimpl.h"
#include "networkserverimpl.h"
#include "networkhttpimpl.h"
#include "networkhttpserviceimpl.h"


namespace rapid
{

	network_engine_impl::network_engine_impl()
	{
#if (RAPID_PLATFORM == RAPID_PLATFORM_WIN32)
		WSADATA wsaData;
		WSAStartup(0x101, &wsaData);
#endif
		//event_enable_debug_logging(EVENT_DBG_ALL);

	}

	network_engine_impl::~network_engine_impl()
	{
#if (RAPID_PLATFORM == RAPID_PLATFORM_WIN32)
		WSACleanup();
#endif
	}

	network_client* network_engine_impl::create_client()
	{
		return new network_client_impl;
	}

	void network_engine_impl::destory_client(network_client& client)
	{
		delete static_cast<network_client_impl*>(&client);
	}

	network_server* network_engine_impl::create_server()
	{
		return new network_server_impl;
	}

	void network_engine_impl::destory_server(network_server& server)
	{
		delete static_cast<network_server_impl*>(&server);
	}

	network_http * network_engine_impl::create_http()
	{
		return new network_http_impl;
	}

	void network_engine_impl::destory_http(network_http & http)
	{
		delete static_cast<network_http_impl*>(&http);
	}

	network_http_service * network_engine_impl::create_http_service()
	{
		return new network_http_service_impl;
	}

	void network_engine_impl::destory_http_service(network_http_service & service)
	{
		delete static_cast<network_http_service_impl*>(&service);
	}


	void network_engine_impl::update(r_uint32 t)
	{

	}

}