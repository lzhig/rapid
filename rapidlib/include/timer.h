#pragma once
#include "types_def.h"
#include <list>
#include <map>
#include <functional>

namespace rapidlib
{
	enum timer_type
	{
		timer_nerver,		// 不超时
		timer_once,			// 激活一次
		timer_repeat,		// 重复激活
	};

	class r_timer;

	typedef std::function<void(r_timer* timer)>	timer_timeout_callback;

	class r_timer
	{
		friend class timer_manager;
	public:


		r_timer(timer_type type, r_uint32 interval, void* binding_object, timer_timeout_callback& callback)
			: m_type(type), m_interval(interval), m_binding_object(binding_object), m_callback(callback)
		{
		}
		r_timer(timer_type type, r_uint32 interval, void* binding_object, timer_timeout_callback&& callback)
			: m_type(type), m_interval(interval), m_binding_object(binding_object), m_callback(callback)
		{
		}

	protected:
		void set_start_time(r_uint32 t) { m_start_time = t; m_end_time = m_start_time + m_interval; }
		r_uint32 get_end_time() const { return m_end_time; }
		timer_timeout_callback& timer_callback() { return m_callback; }
		timer_type get_type() const { return m_type; }
		void* get_binding_object() const { return m_binding_object; }

		static bool compare(const r_timer* t1, const r_timer* t2)
		{
			return t1->m_end_time < t2->m_end_time;
		}

	private:
		void*						m_binding_object = nullptr;
		timer_type					m_type = timer_nerver;
		r_uint32					m_interval = 0;
		r_uint32					m_start_time = 0;
		r_uint32					m_end_time = 0;
		timer_timeout_callback		m_callback = nullptr;
	};

	class timer_manager
	{
	public:

		timer_manager() {}
		~timer_manager();

		r_timer* create_timer(timer_type type, r_uint32 interval, void* bind_object, timer_timeout_callback&& callback);
		r_timer* create_timer(timer_type type, r_uint32 interval, void* bind_object, timer_timeout_callback& callback);
		void destroy_timer(r_timer* timer);

		void* get_bind_object(r_timer* timer);

		void update(r_uint32 t);

	private:

		void _remove_timer_list(r_timer* t);
		void _remove_timer(r_timer* t);

		void _sort_timeout_list();
		void _delay_remove_timer_list();


		typedef std::list<r_timer*>	timer_list;
		timer_list					m_timer_timeout_list;
		timer_list					m_timer_timeout_remove_list;

		typedef std::map<r_timer*, r_timer*>	timer_map;
		timer_map							m_timers;

		r_uint32							m_now = 0;
	};
}