#ifndef __OOP_HAL_H__
#define __OOP_HAL_H__



#include "assert.h"


/*class defination header*/
#define DEF_CLASS(__NAME)\
	typedef struct __##__NAME	__##__NAME;\
	struct  __##__NAME{

#define END_DEF_CLASS(__NAME)\
	};\
	INIT_DECLARATION(__NAME);

/*transfer struct to class to regaring c++*/
#define CLASS(__NAME)	  __##__NAME



/*implement the function of new in c++*/
#define NEW(__CLASSP, __NAME) \
	do{\
		__CLASSP = (CLASS(__NAME)*)os_malloc(sizeof(CLASS(__NAME)));\
		if (init_##__NAME(__CLASSP) == FALSE){\
			os_free(__CLASSP);\
		}\
	 }\
	while(0)
		
#define DELETE(__CLASSP, __NAME) \
	do{\
		if (__CLASSP != NULL){\
			((CLASS(__NAME)*)(__CLASSP))->de_init(((CLASS(__NAME)*)(__CLASSP)));\
			(__CLASSP) = NULL;\
	 	}\
	  }while(0)

/*initiate function declaration*/
#define INIT_DECLARATION(__NAME)\
	extern bool init_##__NAME(CLASS(__NAME) *arg)


#endif
