#ifndef rapid_h__
#define rapid_h__

#include "platform.h"

#include <cassert>
#include <iostream>

//生成服务插件时需要定义该宏，使用不要定义
#ifdef SERVICE_EXPORTS
#	ifdef _MSC_VER
#		define SERVICE_API	__declspec( dllexport )
#	else //gcc 4.x+
#		define SERVICE_API	__attribute__((__visibility__("default")))
#	endif //vc + gcc
#else
#	define SERVICE_API
#endif //SERVICE_EXPORTS

namespace rapidlib
{
#if	defined( RAPID_DEBUG_MODE )
#	define  r_assert( X )			assert( X )
#elif defined( RAPID_FINAL_MODE )
#	define  r_assert( X )
#else	//!RAPID_FINAL_MODE
	static inline void __rapid_assert_r__( bool b,const char *f,unsigned int l,const char *m,const char *x )
	{
		if ( !b ) std::cerr << "!!!!! Error : " << f << "," << l << "," << m << " " << x << " !!!!!" << std::endl;
	}
#	define  r_assert( X )			__rapid_assert_r__( X,__FILE__,__LINE__,__FUNCTION__,#X )
#endif	//!RAPID_DEBUG_MODE

#define r_delete(x)					{if(x) {delete (x);(x)=nullptr;}}
#define r_delete_array(x)			{if(x) {delete [] (x);(x)=nullptr;}}

}

#endif // rapid_h__
