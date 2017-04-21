#ifndef service_manager_h__
#define service_manager_h__

#include <string>

namespace rapidlib
{
	class service_manager
	{
	public:
		virtual void* get_service(const std::string& name) = 0;
		virtual bool register_service(const std::string& name, void* service) = 0;
		virtual void unregister_service(const std::string& name) = 0;
	};

}

#endif // service_manager_h__
