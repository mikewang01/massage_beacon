/*
 *  Copyright (c) 2010 - 2011 Espressif System
 *
 */

#ifndef _ASSERT_H_
#define _ASSERT_H_

#define CLING_DEBUG(format, ...) os_printf(format, ##__VA_ARGS__)

#undef assert
#ifdef NDEBUG
#define assert(__exp) ((void)0)
#else
#define assert(__exp) do{												\
	if(!(__exp))														\
		{																		\
			CLING_DEBUG("Assertion failed:file %s,line %d\n",__FILE__,__LINE__);\
			return FALSE;														\
		}																		\
	}while(0)
#endif/*unacceptable*/


#endif /* _ASSERT_H_ */
