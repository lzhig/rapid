#ifndef lib_manager_h__
#define lib_manager_h__

#include "types_def.h"
#include <string>

using namespace rapidlib;

namespace rapidlib
{
	class lib_manager
	{
	public:
		virtual r_bool load_libraries(const std::string& config_file) = 0;

		virtual r_bool load(const std::string& filename) = 0;
		virtual void unload() = 0;
		virtual r_bool startup() = 0;
		virtual void shutdown() = 0;
		virtual void update(r_uint32 t) = 0;
	};
}

#endif // lib_manager_h__
