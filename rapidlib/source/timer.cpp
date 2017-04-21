#include "timer.h"
#include <algorithm>


namespace rapidlib
{

	timer_manager::~timer_manager()
	{
		for (auto it = m_timers.begin(); it != m_timers.end(); ++it)
		{
			auto t = it->second;
			delete t;
		}
	}

	r_timer* timer_manager::create_timer(timer_type type, r_uint32 interval, void* bind_object, timer_timeout_callback&& callback)
	{
		auto t = new r_timer(type, interval, bind_object, callback);
		auto ret = m_timers.insert(std::make_pair(t, t));

		if (!ret.second)
		{
			delete t;
			return 0;
		}

		// add time list
		t->set_start_time(m_now);
		if (type != timer_nerver)
		{
			m_timer_timeout_list.push_back(t);

			// sort order
			_sort_timeout_list();
		}


		return t;
	}

	r_timer* timer_manager::create_timer(timer_type type, r_uint32 interval, void* bind_object, timer_timeout_callback& callback)
	{
		return create_timer(type, interval, bind_object, std::move(callback));
	}

	void timer_manager::destroy_timer(r_timer* timer)
	{
		_remove_timer_list(timer);
		_remove_timer(timer);
	}

	void* timer_manager::get_bind_object(r_timer* timer)
	{
		auto it = m_timers.find(timer);
		return it == m_timers.end() ? nullptr : it->second->get_binding_object();
	}

	void timer_manager::update(r_uint32 t)
	{
		// check timeout
		m_now += t;

		r_bool need_sort_timeout_list = false;

		timer_list need_notify_timers;

		for (auto it = m_timer_timeout_list.begin(); it != m_timer_timeout_list.end(); ++it)
		{
			auto t = *it;
			if (t->get_end_time() < m_now)
			{
				// timeout, notify later
				need_notify_timers.push_back(t);

				if (t->get_type() == timer_once)
				{
					// prepare to remove it
					m_timer_timeout_remove_list.push_back(t);
				}
				else
				{
					t->set_start_time(m_now);

					need_sort_timeout_list = true;
				}
			}
			else
				break;
		}

		_delay_remove_timer_list();

		// notify
		std::for_each(need_notify_timers.begin(), need_notify_timers.end(), [](r_timer* t)
		{
			t->timer_callback()(t);
		});


		if (need_sort_timeout_list)
		{
			_sort_timeout_list();
		}
	}

	void timer_manager::_remove_timer_list(r_timer* t)
	{
		auto it = std::find(m_timer_timeout_list.begin(), m_timer_timeout_list.end(), t);
		if (m_timer_timeout_list.end() != it)
			m_timer_timeout_list.erase(it);
	}

	void timer_manager::_remove_timer(r_timer* t)
	{
		auto it = m_timers.find(t);
		if (it != m_timers.end())
		{
			auto t = it->second;
			m_timers.erase(it);
			delete t;
		}
	}

	void timer_manager::_sort_timeout_list()
	{
		//std::sort(m_timer_timeout_list.begin(), m_timer_timeout_list.end(), timer());
		m_timer_timeout_list.sort(&r_timer::compare);

	}

	void timer_manager::_delay_remove_timer_list()
	{
		for (auto it = m_timer_timeout_remove_list.begin(); it != m_timer_timeout_remove_list.end(); ++it)
		{
			_remove_timer_list(*it);
		}
		m_timer_timeout_remove_list.clear();
	}

}
