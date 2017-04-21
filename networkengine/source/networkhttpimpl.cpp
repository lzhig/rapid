#include "networkhttpimpl.h"

#include <event2/http.h>
#include <event2/http_struct.h>
#include <algorithm>
#include "event2/buffer.h"
#include "rapid.h"
#include <string.h>
#include <sstream>

rapid::network_http_impl::network_http_impl()
{
	m_event_base = event_base_new();
}

rapid::network_http_impl::~network_http_impl()
{
	event_base_free(m_event_base);
}

void * rapid::network_http_impl::open_url(const char * url, r_bool url_is_RFC3986, method_type method, std::map<const char*, const char*>* properties, void * pdata, r_uint32 len, r_uint32 timeout, r_uint32 retries, open_url_callback && callback)
{
	auto http_uri = url_is_RFC3986 ? evhttp_uri_parse(url) : evhttp_uri_parse_with_flags(url, EVHTTP_URI_NONCONFORMANT);
	if (!http_uri)
		return nullptr;

	struct evhttp_connection* _conn = nullptr;
	struct evhttp_request* _http_request = nullptr;
	struct r_request* _request = nullptr;

	auto _free = [&]() {
		evhttp_uri_free(http_uri);
		if (_conn)
			evhttp_connection_free(_conn);

		if (_http_request)
			evhttp_request_free(_http_request);

		if (_request)
			delete _request;
	};

	do
	{
		auto scheme = evhttp_uri_get_scheme(http_uri);
		auto host = evhttp_uri_get_host(http_uri);
		auto path = evhttp_uri_get_path(http_uri);
		auto port = evhttp_uri_get_port(http_uri);

		if (port == -1)
			port = 80;

		_conn = evhttp_connection_base_new(m_event_base, nullptr, host, port);
		if (!_conn)
			break;


		_request = new r_request(this, _conn, callback);
		if (!_request)
			break;

		evhttp_connection_set_timeout(_conn, timeout);
		evhttp_connection_set_retries(_conn, retries);

		_http_request = evhttp_request_new(network_http_impl::REQUEST_COMPLETE_CALLBACK, _request);
		if (!_http_request)
			break;
		_request->m_http_request = _http_request;


		evhttp_request_set_chunked_cb(_http_request, network_http_impl::REQUEST_CHUNK_CALLBACK);


		if (evhttp_add_header(_http_request->output_headers, "Host", host) != 0)
			break;

		if ((properties == nullptr || properties->find("Content-Type") == properties->end())
			&& _convert_method(method) == EVHTTP_REQ_POST)
		{
			if (evhttp_add_header(_http_request->output_headers, "Content-Type", "application/x-www-form-urlencoded") != 0)
				break;
		}

		if (properties)
		{
			for (auto it = properties->begin(); it != properties->end(); ++it)
			{
				if (evhttp_add_header(_http_request->output_headers, it->first, it->second) != 0)
				{
					_free();
					return nullptr;
				}
			}
		}

		if (pdata && len > 0)
		{
			auto buff = evhttp_request_get_output_buffer(_http_request);
			evbuffer_add(buff, pdata, len);
		}

		if (url_is_RFC3986)
		{
			if (evhttp_make_request(_conn, _http_request, _convert_method(method), path) != 0 || !_request->m_request_valid)
				break;
		}
		else
		{
			std::stringstream ss;
			ss << path;

			auto query = evhttp_uri_get_query(http_uri);
			if (query)
				ss << "?" << query;

			if (evhttp_make_request(_conn, _http_request, _convert_method(method), ss.str().c_str()) != 0 || !_request->m_request_valid)
				break;
		}

		evhttp_uri_free(http_uri);
		m_requests.push_back(_request);

		return _request;

	} while (false);

	_free();

	return nullptr;
}

void * rapid::network_http_impl::open_url(const char * url, r_bool url_is_RFC3986, method_type method, std::map<const char*, const char*>* propertys, void * pdata, r_uint32 len, r_uint32 timeout, r_uint32 retries, open_url_callback & callback)
{
	return open_url(url, url_is_RFC3986, method, propertys, pdata, len, timeout, retries, std::move(callback));
}

void rapid::network_http_impl::update(r_uint32 t)
{
	event_base_loopexit(m_event_base, &m_timev);
	event_base_dispatch(m_event_base);
}

void rapid::network_http_impl::REQUEST_COMPLETE_CALLBACK(struct evhttp_request* http_req, void* pArgs)
{
	auto _request = static_cast<r_request*>(pArgs);
	_request->m_http->_request_complete(http_req, _request);
}

void rapid::network_http_impl::REQUEST_CHUNK_CALLBACK(struct evhttp_request* http_req, void* pArgs)
{
	auto _request = static_cast<r_request*>(pArgs);
	_request->m_http->_request_chunk(http_req, _request);
}

void rapid::network_http_impl::_request_complete(struct evhttp_request* http_req, r_request* _request)
{
	auto it = std::find(m_requests.begin(), m_requests.end(), _request);
	if (it == m_requests.end())
	{
		_request->m_request_valid = false;
	}
	else
	{
		m_requests.erase(it);
		int nErrorCode = evhttp_request_get_response_code(_request->m_http_request);
		if (nErrorCode == 200)
		{
			_request->m_callback(true, _request, (void*)_request->m_data.c_str(), _request->m_data.length());
		}
		else
		{
			_request->m_callback(false, _request, nullptr, 0);
		}
		evhttp_connection_free(_request->m_http_connection);
		delete _request;
	}
}

void rapid::network_http_impl::_request_chunk(struct evhttp_request* http_req, r_request* _request)
{
	evbuffer* evbuff = evhttp_request_get_input_buffer(_request->m_http_request);
	int nLength = (int)evbuffer_get_length(evbuff);
	if (nLength > 0)
	{
		while (nLength > m_open_url_temp_length)
		{
			evbuffer_remove(evbuff, m_open_url_temp_buf, m_open_url_temp_length);
			_request->m_data += std::string(m_open_url_temp_buf, m_open_url_temp_length);
			nLength -= m_open_url_temp_length;
		}
		evbuffer_remove(evbuff, m_open_url_temp_buf, nLength);
		_request->m_data += std::string(m_open_url_temp_buf, nLength);
	}
}
