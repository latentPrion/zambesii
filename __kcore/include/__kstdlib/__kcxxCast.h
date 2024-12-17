#ifndef ___KCXX_CAST_H
	#define ___KCXX_CAST_H

#define R_CAST(__type,__var)		reinterpret_cast<__type>( __var )
#define C_CAST(__type,__var)		const_cast<__type>( __var )
#define S_CAST(__type,__var)		static_cast<__type>( __var )
#define D_CAST(__type,__var)		dynamic_cast<__type>( __var )

#endif

