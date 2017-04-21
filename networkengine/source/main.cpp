#include "rapid.h"
#include <iostream>
#include "framework.h"
#include "networkengineimpl.h"

using namespace rapidlib;

extern "C"
{
	rapid::network_engine_impl g_network_engine;
	framework* g_framework = NULL;

	SERVICE_API r_bool startup(framework& fw)
	{
		//std::cout << "startup..." << std::endl;
		g_framework = &fw;
		g_framework->get_service_manager().register_service(RAPID_NETWORK_ENGINE, &g_network_engine);
		
		return true;
	}

	SERVICE_API void shutdown(void)
	{
		//std::cout << "shutdown..." << std::endl;
		g_framework->get_service_manager().unregister_service(RAPID_NETWORK_ENGINE);
	}

	SERVICE_API void update(r_uint32 t)
	{
		//std::cout << "update..." << std::endl;
		g_network_engine.update(t);
	}
};