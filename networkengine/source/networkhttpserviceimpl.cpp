#include "networkhttpserviceimpl.h"
#include <event2/thread.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>
#include <iostream>
#include <cstring>

namespace rapid
{
	network_http_service_impl::network_http_service_impl()
	{
	}

	network_http_service_impl::~network_http_service_impl()
	{
	}

	r_bool network_http_service_impl::start(const char * host, r_uint16 port, http_request_callback && callback)
	{
		m_callback = callback;

#if (RAPID_PLATFORM == RAPID_PLATFORM_WIN32)
		evthread_use_windows_threads();
		int configflag = EVENT_BASE_FLAG_STARTUP_IOCP;
#else
		evthread_use_pthreads();
		int configflag = EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST;
#endif
		struct event_config* eventconfig = nullptr;

		do 
		{
			eventconfig = event_config_new();
			if (!eventconfig)
			{
				std::cout << "[network_http_service]: event_config_new() failed!!!" << std::endl;
				break;
			}


			//event_config_set_flag(eventconfig, configflag);
			event_config_set_num_cpus_hint(eventconfig, 4);

			m_event_base = event_base_new_with_config(eventconfig);
			if (!m_event_base)
			{
				std::cout << "[network_http_service]: event_base_new_with_config() failed!!!" << std::endl;
				break;
			}

			m_http = evhttp_new(m_event_base);
			if (!m_http)
			{
				std::cout << "[network_http_service]: evhttp_new() failed!!!" << std::endl;
				break;
			}

			evhttp_set_gencb(m_http, network_http_service_impl::HTTP_REQUEST_CALLBACK, this);
			if (!evhttp_bind_socket_with_handle(m_http, host, port))
			{
				std::cout << "[network_http_service]: could not bind to " << host << ":" << port << std::endl;
				break;
			}

			return true;

		} while (false);

		if (eventconfig)
			event_config_free(eventconfig);

		shutdown();

		return false;
	}
	void network_http_service_impl::shutdown()
	{
		if (m_event_base)
		{
			event_base_free(m_event_base);
			m_event_base = nullptr;
		}

		if (m_http)
		{
			evhttp_free(m_http);
			m_http = nullptr;
		}
	}

	void network_http_service_impl::update(r_uint32 t)
	{
		if (m_event_base)
		{
			event_base_loopexit(m_event_base, &m_timev);
			event_base_dispatch(m_event_base);
		}
	}

	void network_http_service_impl::send_response(void* request_handle, r_uint32 code, const char * reason, const void * data, r_uint32 len)
	{
		struct evhttp_request* evreq = (evhttp_request*)request_handle;
		struct evbuffer* evoutput = evbuffer_new();
		evbuffer_add(evoutput, data, len);
		evhttp_send_reply(evreq, code, reason, evoutput);
		evbuffer_free(evoutput);

		if (evhttp_request_is_owned(evreq) == 1)
		{
			evhttp_request_free(evreq);
		}
	}

	void network_http_service_impl::HTTP_REQUEST_CALLBACK(evhttp_request * evreq, void * pArgs)
	{
		static_cast<network_http_service_impl*>(pArgs)->_handle_http_request(evreq);
	}

	void rapid::network_http_service_impl::_handle_http_request(evhttp_request * evreq)
	{
		struct evbuffer* evinput = evhttp_request_get_input_buffer(evreq);
		auto length = (int)evbuffer_get_length(evinput);
		auto pData = new unsigned char[length];
		memset(pData, 0, length);
		evbuffer_remove(evinput, pData, length);

		m_callback(evreq, evhttp_request_get_uri(evreq), pData, length);

		delete pData;
	}
}