#ifndef framework_h__
#define framework_h__

#include <string>

#include "types_def.h"
#include "service_manager.h"
#include "lib_manager.h"

namespace rapidlib
{
	class framework
	{
	public:
		//virtual r_bool load_library_from_config(const std::string& filename) = 0;

		virtual void run() = 0;

		virtual void quit() = 0;

		virtual service_manager& get_service_manager() = 0;

		virtual lib_manager& get_lib_manager() = 0;
	};

}

#endif // framework_h__
