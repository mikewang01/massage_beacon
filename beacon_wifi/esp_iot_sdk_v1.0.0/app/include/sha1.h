#ifndef __SHA1_H__
#define __SHA1_H__

#include "ets_sys.h"
#include "osapi.h"
#include "C_types.h"



#if !defined(SHA1_UTILITY_FUNCTIONS) && !defined(SHA1_NO_UTILITY_FUNCTIONS)
#define SHA1_UTILITY_FUNCTIONS
#endif



// You can define the endian mode in your files, without modifying the SHA1
// source files. Just #define SHA1_LITTLE_ENDIAN or #define SHA1_BIG_ENDIAN
// in your files, before including the SHA1.h header file. If you don't
// define anything, the class defaults to little endian.

#if !defined(SHA1_LITTLE_ENDIAN) && !defined(SHA1_BIG_ENDIAN)
#define SHA1_LITTLE_ENDIAN
#endif

// Same here. If you want variable wiping, #define SHA1_WIPE_VARIABLES, if
// not, #define SHA1_NO_WIPE_VARIABLES. If you don't define anything, it
// defaults to wiping.

#if !defined(SHA1_WIPE_VARIABLES) && !defined(SHA1_NO_WIPE_VARIABLES)
#define SHA1_WIPE_VARIABLES
#endif

#ifdef SHA1_UTILITY_FUNCTIONS
	// Two different formats for ReportHash(...)
	enum
	{
		REPORT_HEX = 0,
		REPORT_DIGIT = 1
	};

#endif




/////////////////////////////////////////////////////////////////////////////
// Declare SHA1 workspace


typedef union
{
	uint8  c[64];
	uint32 l[16];
} SHA1_WORKSPACE_BLOCK;



void csha1_init(void);
void csha1_reset(void);

/*Update the hash value*/
void csha1_update(uint8 *data, uint32 len);
/* Finalize hash and report*/
void csha1_final(void);

	// Report functions: as pre-formatted and raw data
#ifdef SHA1_UTILITY_FUNCTIONS
	void csha1_reportHash(char *szReport, unsigned char uReportType);
#endif

void csha1_getHash(uint8 *puDest);



	


	



#endif

