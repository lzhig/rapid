#include "service_manager_impl.h"
#include "rapid.h"

namespace rapidlib
{

	void* service_manager_impl::get_service(const std::string& name)
	{
		auto it = m_services.find(name);
		return it == m_services.end() ? NULL : it->second;
	}

	bool service_manager_impl::register_service(const std::string& name, void* service)
	{
		return m_services.insert(std::make_pair(name, service)).second;
	}

	void service_manager_impl::unregister_service(const std::string& name)
	{
		auto it = m_services.find(name);
		if (it == m_services.end())
		{
			r_assert(false);
			return;
		}

		m_services.erase(it);
	}

}
