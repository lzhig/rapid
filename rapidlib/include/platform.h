#ifndef platform_h__
#define platform_h__

namespace rapidlib
{
	/*
 * Initial platform/compiler-related stuff to set.
 */
#define RAPID_PLATFORM_WIN32		1
#define RAPID_PLATFORM_LINUX		2
#define RAPID_PLATFORM_APPLE		3
#define RAPID_PLATFORM_FREEBSD		4

#define RAPID_COMPILER_MSVC			1
#define RAPID_COMPILER_GNUC			2
#define RAPID_COMPILER_BORL			3

#define RAPID_ENDIAN_LITTLE			1
#define RAPID_ENDIAN_BIG			2

#define RAPID_ARCHITECTURE_32		1
#define RAPID_ARCHITECTURE_64		2

//----------------------------------------------------------------------------
// Endian Settings
// check for BIG_ENDIAN config flag, set RAPID_ENDIAN correctly
#ifdef CONFIG_BIG_ENDIAN
#    define RAPID_ENDIAN	RAPID_ENDIAN_BIG
#else
#    define RAPID_ENDIAN	RAPID_ENDIAN_LITTLE
#endif

/*
 * Finds the compiler type and version.
 */
#if defined( _MSC_VER )
#   define RAPID_COMPILER	RAPID_COMPILER_MSVC
#   define RAPID_COMP_VER	_MSC_VER
#elif defined( __GNUC__ )
#   define RAPID_COMPILER	RAPID_COMPILER_GNUC
#   define RAPID_COMP_VER	(((__GNUC__)*100) + (__GNUC_MINOR__*10) + __GNUC_PATCHLEVEL__)
#elif defined( __BORLANDC__ )
#   define RAPID_COMPILER	RAPID_COMPILER_BORL
#   define RAPID_COMP_VER	__BCPLUSPLUS__
#else
#   pragma error "No known compiler. Abort! Abort!"
#endif	//RAPID_COMPILER

/* See if we can use __forceinline or if we need to use __inline instead */
#if RAPID_COMPILER != RAPID_COMPILER_MSVC
#   define FORCEINLINE		__inline
#else
#   if RAPID_COMP_VER >= 1200
#       define	FORCEINLINE	__forceinline
#   endif
#endif //RAPID_COMPILER_MSVC

/* Finds the current platform */
#if defined( __WIN32__ ) || defined( _WIN32 )
#	define RAPID_PLATFORM	RAPID_PLATFORM_WIN32
#	define __WINDOWS__
#elif defined( __APPLE_CC__ )
#	define RAPID_PLATFORM	RAPID_PLATFORM_APPLE
#	define __MACOSX__
#elif defined( __FreeBSD__ )
#	define RAPID_PLATFORM	RAPID_PLATFORM_FREEBSD
#else
#	define RAPID_PLATFORM	RAPID_PLATFORM_LINUX
#	define __LINUX__
#endif //RAPID_PLATFORM

/* Find the arch type */
#if defined(__x86_64__) || defined(_M_X64)
#   define RAPID_ARCH_TYPE	RAPID_ARCHITECTURE_64
#else
#   define RAPID_ARCH_TYPE	RAPID_ARCHITECTURE_32
#endif //RAPID_ARCH_TYPE

	// Win32 compilers use _DEBUG for specifying debug builds.
	// Linux compilers seem to use DEBUG for when specifying a debug build.
#if	defined( _DEBUG ) || defined( DEBUG )
#	define RAPID_DEBUG_MODE
#endif //windows _DEBUG || unix DEBUG

#ifdef __WINDOWS__
//----------------------------------------------------------------------------
// Windows Settings

#if !defined(__cplusplus) // < 201103L

#	ifndef	__MINGW32__
#		define snprintf		_snprintf
#		define vsnprintf	_vsnprintf
#		define atoll		_atoi64
#	endif	//__MINGW32__

#endif

#else //__WINDOWS__
//----------------------------------------------------------------------------
// Linux/Apple Settings

// A quick define to overcome different names for the same function
#   define stricmp strcasecmp

#endif //!__WINDOWS__

}

#endif // platform_h__
