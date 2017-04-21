#ifndef std_headers_h__
#define std_headers_h__


#	ifdef __BORLANDC__
#		define __STD_ALGORITHM
#	endif //__BORLANDC__

#	include <cassert>
#	include <cstdio>
#	include <cstdlib>
#	include <ctime>
#	include <cstring>
#	include <cstdarg>
#	include <cmath>

// STL containers
#	include <vector>
#	include <map>
#	include <string>
#	include <set>
#	include <list>
#	include <deque>
#	include <queue>
#	include <bitset>

// STL algorithms & functions
#	include <algorithm>
#	include <functional>
#	include <limits>

// C++ Stream stuff
#	include <fstream>
#	include <iostream>
#	include <iomanip>
#	include <sstream>

#	ifdef __BORLANDC__
	namespace rapidlib
	{
		using namespace std;
	}
#	endif //__BORLANDC__



#	ifdef __WINDOWS__
#		define WIN32_LEAN_AND_MEAN
#		ifndef	_WIN32_WINNT
#			define _WIN32_WINNT	0x0500
#		endif	//_WIN32_WINNT
#		include <windows.h>
#		undef min
#		undef max
#		if defined( __MINGW32__ )
#			include <unistd.h>
#		endif
#	else
	extern "C" {
#		include <unistd.h>
	}
#	endif //__WINDOWS__

#endif // std_headers_h__
