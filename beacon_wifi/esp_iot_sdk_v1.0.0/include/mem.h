#ifndef __MEM_H__
#define __MEM_H__

//void *pvPortMalloc( size_t xWantedSize );
//void vPortFree( void *pv );
//void *pvPortZalloc(size_t size);

#define os_malloc   pvPortMalloc
#define os_free(__x) do{\
if((__x) != NULL){\
	vPortFree((__x));\
	(__x) = NULL;\
 }\
}while(0)\


#define os_zalloc   pvPortZalloc

#endif
