#pragma once

#include "types_def.h"
#include <map>
#include <functional>

using namespace rapidlib;

namespace rapid
{

#define RAPID_NETWORK_ENGINE		"rapid_networkengine"

	class network_client_listener
	{
	public:
		virtual void server_connected(void) = 0;		// 服务器连接成功

		virtual void connect_Failed(void) = 0;			// 由于无法连接,连接服务器失败

		virtual void server_full(void) = 0;				// 服务器已满

		virtual void server_disconnected(void) = 0;		// 服务器断开了与我们的连接

		virtual void server_lost(void) = 0;				// 不能向服务器投递可靠包,同服务器的连接丢失了

		virtual void server_message(void* data, r_uint32 len) = 0;		// message from server
	};

	class network_client
	{
	public:
		virtual r_bool connect(const char* host, r_uint16 port) = 0;
		virtual void disconnect(r_bool wait = true, r_bool notify = false) = 0;

		virtual void add_listener(network_client_listener& listener) = 0;
		virtual void remove_listener(network_client_listener& listener) = 0;

		virtual void set_timeout(r_uint32 time) = 0;

		virtual r_bool is_connected() const = 0;

		virtual void handle_message() = 0;

		virtual void send(void* data, r_uint32 len) = 0;
	};

	class network_server_listener
	{
	public:
		virtual void client_connected(void *net) = 0;
		virtual void client_disconnected(void *net) = 0;
		virtual void client_lost(void *net) = 0;

		virtual void client_message(void *net, void* data, r_uint32 len) = 0;
	};

	class network_server
	{
	public:
		virtual r_bool start(const char* host, r_uint16 port, r_uint32 max_connections) = 0;
		virtual void stop(r_uint32 wait_time = 0) = 0;

		virtual void disconnect(void* net, bool notify = true) = 0;
		virtual void disconnect_all_clients(r_bool notify = true) = 0;


		virtual void add_listener(network_server_listener& listener) = 0;
		virtual void remove_listener(network_server_listener& listener) = 0;

		virtual void set_binding_data(void* net, void* data) = 0;
		virtual void* get_binding_data(void* net) = 0;

		virtual void set_timeout(r_uint32 time) = 0;

		virtual r_bool is_running() const = 0;

		virtual void handle_message() = 0;

		virtual void send(void* net, void* data, r_uint32 len) = 0;
	};

	class network_http
	{
	public:
		enum method_type
		{
			method_get,
			method_post,
		};
		typedef std::function<void(r_bool ret, void* request, void* data, r_uint32 len)>	open_url_callback;
		virtual void* open_url(const char* url, r_bool url_is_RFC3986, method_type method, std::map<const char*, const char*>* propertys, void* pdata, r_uint32 len, r_uint32 timeout, r_uint32 retries, open_url_callback&& callback) = 0;
		virtual void* open_url(const char* url, r_bool url_is_RFC3986, method_type method, std::map<const char*, const char*>* propertys, void* pdata, r_uint32 len, r_uint32 timeout, r_uint32 retries, open_url_callback& callback) = 0;
		virtual void update(r_uint32 t) = 0;
	};

	class network_http_service
	{
	public:
		typedef std::function<void(void* request_handle, const char* url, const void* data, r_uint32 len)>				http_request_callback;
		virtual r_bool start(const char* host, r_uint16 port, http_request_callback&& callback) = 0;
		virtual void shutdown() = 0;
		virtual void update(r_uint32 t) = 0;
		virtual void send_response(void* request_handle, r_uint32 code, const char* reason, const void* data, r_uint32 len) = 0;
	};

	class network_engine
	{
	public:
		virtual network_client* create_client() = 0;
		virtual void destory_client(network_client& client) = 0;

		virtual network_server* create_server() = 0;
		virtual void destory_server(network_server& server) = 0;

		virtual network_http* create_http() = 0;
		virtual void destory_http(network_http& http) = 0;

		virtual network_http_service* create_http_service() = 0;
		virtual void destory_http_service(network_http_service& service) = 0;

		virtual void update(r_uint32 t) = 0;
	};
}