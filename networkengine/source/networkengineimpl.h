#pragma once

#include "../include/networkengine.h"

namespace rapid
{

	class network_engine_impl : public network_engine
	{
	public:
		network_engine_impl();
		~network_engine_impl();

		// Í¨¹ý network_engine ¼Ì³Ð
		virtual network_client* create_client() override;
		virtual void destory_client(network_client& client) override;

		virtual network_server* create_server() override;
		virtual void destory_server(network_server& server) override;

		virtual network_http * create_http() override;
		virtual void destory_http(network_http & http) override;

		virtual network_http_service * create_http_service() override;
		virtual void destory_http_service(network_http_service & service) override;

		virtual void update(r_uint32 t) override;
	};
}