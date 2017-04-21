#include "framework_impl.h"
#include <iostream>

using namespace rapidlib;

int main(int argc,char *argv[])
{
	framework_impl g_framework;

	if (argc > 1 && !g_framework.get_lib_manager().load_libraries(argv[1]))
	{
		std::cout << "failed to load library config file \"" << argv[1] << "\"." << std::endl;
		g_framework.quit();
	}

	g_framework.run();

	return 0;
}

