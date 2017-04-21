#pragma once

#include "lib_manager.h"
#include "service_manager.h"
#include "framework.h"
#include "service_manager_impl.h"
#include "lib_manager_impl.h"

namespace rapidlib
{
	class framework_impl : public framework
	{
	public:
		framework_impl();
		~framework_impl();

		// Í¨¹ý framework ¼Ì³Ð
		virtual void run() override;

		virtual service_manager& get_service_manager() override {
			return m_service_manager;
		};

		virtual lib_manager& get_lib_manager() override
		{
			return m_lib_manager;
		}

		virtual void quit() override
		{
			m_running = false;
		}


	private:
		class time_util
		{
		public:
			time_util();
			r_double64 now();

		private:
			static r_double64 m_tick;
		};



	private:
		lib_manager_impl		m_lib_manager;
		service_manager_impl	m_service_manager;

		time_util				m_time;

		r_bool					m_running = true;
	};

}