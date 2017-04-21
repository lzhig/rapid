#pragma once

#include "../include/networkengine.h"

#include <event2/event.h>
#include <vector>
#include "event2/http.h"


using namespace rapidlib;

namespace rapid
{
	class network_http_impl : public network_http
	{
	public:
		network_http_impl();
		~network_http_impl();

		// Í¨¹ý network_http ¼Ì³Ð
		virtual void * open_url(const char * url, r_bool url_is_RFC3986, method_type method, std::map<const char*, const char*>* properties, void * pdata, r_uint32 len, r_uint32 timeout, r_uint32 retries, open_url_callback && callback) override;
		virtual void * open_url(const char * url, r_bool url_is_RFC3986, method_type method, std::map<const char*, const char*>* properties, void * pdata, r_uint32 len, r_uint32 timeout, r_uint32 retries, open_url_callback & callback) override;
		virtual void update(r_uint32 t) override;

	private:

		static void REQUEST_COMPLETE_CALLBACK(struct evhttp_request* http_req, void* pArgs);
		static void REQUEST_CHUNK_CALLBACK(struct evhttp_request* http_req, void* pArgs);



		evhttp_cmd_type _convert_method(method_type type)
		{
			return type == network_http::method_post ? EVHTTP_REQ_POST : EVHTTP_REQ_GET;
		}

		struct r_request
		{
			struct evhttp_connection*					m_http_connection = nullptr;
			struct evhttp_request*						m_http_request = nullptr;
			network_http_impl*							m_http = nullptr;
			open_url_callback							m_callback;
			std::string									m_data;
			r_bool										m_request_valid = true;

			r_request(network_http_impl* http, evhttp_connection* conn, open_url_callback& callback)
				: m_http(http), m_http_connection(conn), m_callback(callback), m_request_valid(true)
			{

			}
		};

		void _request_complete(struct evhttp_request* http_req, r_request* _request);
		void _request_chunk(struct evhttp_request* http_req, r_request* _request);


		typedef std::vector<r_request*>					r_requests;
		r_requests										m_requests;

		struct event_base*								m_event_base = nullptr;

		static const r_uint32 m_open_url_temp_length = 0x10000;
		char m_open_url_temp_buf[m_open_url_temp_length] = {0};

		const struct timeval				m_timev = { 0, 1000 };

	};
}