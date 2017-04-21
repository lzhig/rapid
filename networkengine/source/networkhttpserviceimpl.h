#pragma once

#include "../include/networkengine.h"
#include <event2/event.h>
#include <event2/http.h>

using namespace rapidlib;

namespace rapid
{
	class network_http_service_impl : public network_http_service
	{
	public:
		network_http_service_impl();
		~network_http_service_impl();

		// Í¨¹ý network_http_service ¼Ì³Ð
		virtual r_bool start(const char * host, r_uint16 port, http_request_callback && callback) override;

		virtual void shutdown() override;

		virtual void update(r_uint32 t) override;

		virtual void send_response(void* request_handle, r_uint32 code, const char * reason, const void * data, r_uint32 len) override;

	protected:
		static void HTTP_REQUEST_CALLBACK(struct evhttp_request* pRequest, void* pArgs);
		void _handle_http_request(struct evhttp_request* pRequest);

	private:
		struct event_base*					m_event_base = nullptr;
		struct evhttp*						m_http = nullptr;

		http_request_callback				m_callback;

		const struct timeval				m_timev = {0, 1000};
	};
}