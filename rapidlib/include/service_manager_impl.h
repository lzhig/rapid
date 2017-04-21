#pragma once

#include "service_manager.h"

#include <map>

namespace rapidlib
{
	class service_manager_impl : public service_manager
	{
	public:
		virtual void* get_service(const std::string& name) override;
		virtual bool register_service(const std::string& name, void* service) override;
		virtual void unregister_service(const std::string& name) override;

	private:
		typedef std::map<const std::string, void*>	services_map;
		services_map	m_services;
	};
}
