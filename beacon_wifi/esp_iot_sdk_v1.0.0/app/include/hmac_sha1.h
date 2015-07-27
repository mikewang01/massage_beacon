#ifndef __HMAC_SHA1_H__
#define __HMAC_SHA1_H__

#include "ets_sys.h"
#include "osapi.h"
#include "C_types.h"
#include "oop_hal.h"



#if 0

CLASS(hmac_sha1) 
{
	int a;
	bool (*init)   (CLASS(__NAME) *arg);
 	bool (*set_key)  (CLASS(__NAME) *arg, char *key, size_t size);
 	bool (*set_text) (CLASS(__NAME) *arg, char *text, size_t size);
	bool (*process)  (CLASS(__NAME) *arg, char *output_buffer);
	bool (*to_base64)(char *input, char *output);
};

#else

DEF_CLASS(hmac_sha1)
	bool (*init)    (CLASS(hmac_sha1) *arg);
	bool (*de_init) (CLASS(hmac_sha1) *arg);
 	bool (*set_key) (CLASS(hmac_sha1) *arg, char *key, size_t size);
 	bool (*set_text)(CLASS(hmac_sha1) *arg, char *text, size_t size);
	bool (*process) (CLASS(hmac_sha1) *arg, char *output_buffer, size_t *size);
	bool (*base64_encode) (CLASS(hmac_sha1) *arg, const char *src, size_t src_size, char *dst);
	bool (*base64_decode) (CLASS(hmac_sha1) *arg, char *src, char *dst);
	void *user_data;
END_DEF_CLASS(hmac_sha1)

#endif
/* Hash a single 512-bit block. This is the core of the algorithm.*/




#endif

