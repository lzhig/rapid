#include "lib_manager_impl.h"
#include "rapid.h"
#include "rapidjson/document.h"

namespace rapidlib
{

	//////////////////////////////////////////////////////////////////////////
	
	dynamic_lib::dynamic_lib()
		: m_name(""), m_handle(NULL)
	{

	}

	dynamic_lib::~dynamic_lib()
	{
		if (m_handle)
			unload();
	}

	r_bool dynamic_lib::load(const std::string& name)
	{
		if (m_handle)
			return false;

#ifdef	__LINUX__ //__WINDOWS__ / __MACOSX__
		// dlopen() does not add .so to the filename like windows does for .dll
		if( name.substr(name.length()-3,3) == ".so" )
		{
			m_name = name;
		}
		else
		{
			m_name = name + ".so";
		}
#else	//__WINDOWS__ / __MACOSX__
		if( name.substr(name.length()-4,4) == ".dll" )
		{
			m_name = name;
		}
		else
		{
			m_name = name + ".dll";
		}
#endif	//__LINUX__ / __WINDOWS__ / __MACOSX__

		m_handle = ( DYNLIB_HANDLE ) DYNLIB_LOAD( m_name.c_str() );

		return m_handle != NULL;
	}

	void dynamic_lib::unload(void)
	{
		r_assert(m_handle);
		if (!DYNLIB_UNLOAD(m_handle))
			m_handle = NULL;
	}

	std::string dynamic_lib::get_error(void)
	{
#ifdef	__WINDOWS__
		char buf[1024];
		const DWORD sz = FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, GetLastError(), 0, buf, 1024, NULL );
		return std::string( buf,sz );
#endif	//__WINDOWS__
#ifdef	__LINUX__
		return dlerror();
#endif	//__LINUX__
#ifdef	__MACOSX__
		return dlerror();
#endif	//__MACOSX__
	}

	void* dynamic_lib::get_symbol(const std::string& name) const
	{
		if (!m_handle)
			return NULL;

		return (void*) DYNLIB_GETSYM(m_handle, name.c_str());
	}

	//////////////////////////////////////////////////////////////////////////

	lib_manager_impl::lib_manager_impl()
		: mp_framework(NULL)
	{

	}

	lib_manager_impl::lib_manager_impl(framework* fw)
		: mp_framework(fw)
	{

	}

	lib_manager_impl::~lib_manager_impl()
	{
		shutdown();
	}

	rapidlib::r_bool lib_manager_impl::load_libraries(const std::string& config_file)
	{
		std::ifstream fs(config_file, std::ios::binary | std::ios::in);
		std::string str;
		std::getline(fs, str, (char)EOF);

		rapidjson::Document doc;
		doc.Parse(str.c_str());
		if (doc.IsNull())
			return false;

		rapidjson::Value& val_library = doc["library"];
		if (!val_library.IsArray())
			return false;

		for (auto it = val_library.Begin(); it != val_library.End(); ++it)
		{
			if (!load(it->GetString()))
				return false;
		}

		return true;
	}


	r_bool lib_manager_impl::load(const std::string& filename)
	{
		dynamic_lib* lib = new dynamic_lib();
		if (!lib->load(filename))
		{
			std::clog << "lib manager: failed to load dynamic lib: " << lib->get_name() << "." << std::endl;
			std::clog << "error: " << lib->get_error() << std::endl;
			delete lib;
			return false;
		}

		LIB_FUNC_STARTUP func_startup = (LIB_FUNC_STARTUP)lib->get_symbol("startup");
		LIB_FUNC_SHUTDOWN func_shutdown = (LIB_FUNC_SHUTDOWN)lib->get_symbol("shutdown");
		LIB_FUNC_UPDATE func_update = (LIB_FUNC_UPDATE)lib->get_symbol("update");

		if (func_startup == NULL || func_shutdown == NULL)
		{
			std::clog << "lib manager: failed to load dynamic lib: " << lib->get_name() << "." << std::endl;
			std::clog << "error: not a valid dynamic lib." << std::endl;
			delete lib;
			return false;
		}

		std::clog << "lib manager: startup dynamic lib: " << lib->get_name() << "." << std::endl;
		if (!func_startup(*mp_framework))
		{
			std::clog << "lib manager: failed to startup dynamic lib: " << lib->get_name() << "." << std::endl;
			func_shutdown();
			delete lib;
			return false;
		}


		if (func_update)
			m_libs_update.push_back(func_update);

		m_dynamic_libs.push_back(lib);

		return true;
	}

	void lib_manager_impl::unload()
	{
		r_assert(m_dynamic_libs.size() > 0);

		dynamic_lib* lib = m_dynamic_libs.back();
		m_dynamic_libs.pop_back();

		if (lib->get_symbol("update") != NULL)
			m_libs_update.pop_back();

		LIB_FUNC_SHUTDOWN func_shutdown = (LIB_FUNC_SHUTDOWN)lib->get_symbol("shutdown");
		func_shutdown();

		std::clog << "lib manager: shutdown dynamic lib: " << lib->get_name() << std::endl;
	}

	r_bool lib_manager_impl::startup()
	{
		return true;
	}

	void lib_manager_impl::shutdown()
	{
		if (m_dynamic_libs.empty())
			return;

		m_libs_update.clear();

		for (auto it = m_dynamic_libs.rbegin(); it != m_dynamic_libs.rend(); ++it)
		{
			LIB_FUNC_SHUTDOWN func_shutdown = (LIB_FUNC_SHUTDOWN)(*it)->get_symbol("shutdown");
			func_shutdown();
			std::clog << "lib manager: shutdown dynamic lib: " << (*it)->get_name() << std::endl;
			delete (*it);
		}

		m_dynamic_libs.clear();
	}

	void lib_manager_impl::update(unsigned int t)
	{
		for (auto it = m_libs_update.begin(); it != m_libs_update.end(); ++it)
		{
			(*it)(t);
		}
	}

}
