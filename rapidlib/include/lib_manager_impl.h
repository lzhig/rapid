#pragma once

#include "platform.h"
#include "std_headers.h"
#include "types_def.h"
#include "lib_manager.h"


#ifdef	__WINDOWS__
#	define DYNLIB_HANDLE			HINSTANCE
#	if	_UNICODE
#		define DYNLIB_LOAD( a )		LoadLibraryW( a )
#	else
#		define DYNLIB_LOAD( a )		LoadLibraryA( a )
#	endif
#	define DYNLIB_UNLOAD( a )		!FreeLibrary( a )
#	define DYNLIB_GETSYM( a, b )	GetProcAddress( a, b )
#endif	//__WINDOWS__

#ifdef	__LINUX__
extern "C" {
#	include <dlfcn.h>
}
#	define DYNLIB_HANDLE			void *
#	define DYNLIB_LOAD( a )			dlopen( a, RTLD_LAZY | RTLD_GLOBAL)
#	define DYNLIB_GETSYM( a, b )	dlsym( a, b )
#	define DYNLIB_UNLOAD( a )		dlclose( a )
#endif	//__LINUX__

#ifdef	__MACOSX__
#   define DYNLIB_HANDLE            void*
#   define DYNLIB_LOAD(a)           NULL    // mac_loadDylib(a)
#   define DYNLIB_UNLOAD(a)         true    // mac_unloadExeBundle(a)
#   define DYNLIB_GETSYM(a, b)      NULL    // dlsym(a, b)
#endif	//__MACOSX__


using namespace rapidlib;

namespace rapidlib
{

	//////////////////////////////////////////////////////////////////////////
	// dynamic_lib
	class dynamic_lib
	{
	public:
		dynamic_lib();
		~dynamic_lib();

		r_bool load(const std::string& name);
		void unload(void);

		r_bool is_valid(void) const { return m_handle != NULL; }
		const std::string& get_name(void) const { return m_name; }

		std::string get_error(void);
		void* get_symbol(const std::string& name) const;

	private:
		std::string	m_name;
		DYNLIB_HANDLE		m_handle;
	};

	class framework;

	class lib_manager_impl : public lib_manager
	{
	public:
		lib_manager_impl();
		lib_manager_impl(framework* fw);
		~lib_manager_impl();

		virtual r_bool load(const std::string& filename) override;
		virtual void unload() override;
		virtual r_bool startup() override;
		virtual void shutdown() override;
		virtual void update(r_uint32 t) override;

		virtual r_bool load_libraries(const std::string& config_file) override;


	private:
		typedef r_bool(*LIB_FUNC_STARTUP)(framework&);
		typedef void(*LIB_FUNC_SHUTDOWN)(void);
		typedef void(*LIB_FUNC_UPDATE)(r_uint32);

		framework*					mp_framework = nullptr;

		typedef std::vector<LIB_FUNC_UPDATE>	libs_update;
		libs_update					m_libs_update;

		typedef std::vector<dynamic_lib*>		dynamic_libs;
		dynamic_libs				m_dynamic_libs;
	};

}
