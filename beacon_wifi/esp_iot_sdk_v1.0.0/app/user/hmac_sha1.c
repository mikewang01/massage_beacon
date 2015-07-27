/******************************************************************************
 * Copyright 2015-2016 Hicling Systems (ShangHai)
 *
 * FileName: Hmac_sha1.c
 *
 * Description:  Hmac_sha1 encryption  implemetation.
 *              
 * Modification history:
 *     2015/4/3, v1.0 create this file.   MikeWang
*******************************************************************************/

/*********************************************************************
 * INCLUDES
*/
#include "mem.h"
#include "Hmac_sha1.h"	
#include "sha1.h"
#include "assert.h"
/*********************************************************************
* MACROS
*/




/*********************************************************************
* TYPEDEFS
*/



 enum {
	    SHA1_DIGEST_LENGTH	= 20, /*encryption output data lenth*/
		SHA1_BLOCK_SIZE		= 64, /*ShA1 calculation block size*/
		HMAC_BUF_LEN		= 4096/*maxim size of buffer*/
	 } ;


struct enpryption_information{
	uint8  key[64];   /*store the key parameter used to proceed hmac_sha1 enprytion*/
	size_t key_lenth; /*store the lenth of the key*/
	uint8  text[128]; /*store the data needing enprytion*/
	size_t text_lenth; /*store the lenth of the text*/
};	
	

	
/*********************************************************************
* GLOBAL VARIABLES
*/
	
	
/*********************************************************************
* LOCAL VARIABLES
*/

LOCAL const char Base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*********************************************************************
* EXTERNAL VARIABLES
*/


/*********************************************************************
* FUNCTIONS
*/
LOCAL bool hmac_sha1_set_key (CLASS(hmac_sha1) *arg, char *key, size_t size);
LOCAL bool hmac_sha1_set_text (CLASS(hmac_sha1) *arg, char *text, size_t size);

LOCAL bool hmac_sha1_process_internal(unsigned char *text, int text_len, unsigned char *key, int key_len, unsigned char *digest);
LOCAL bool hmac_sha1_process  (CLASS(hmac_sha1) *arg, char *output_buffer, size_t *size);

LOCAL bool hmac_sha1_base64_decode(CLASS(hmac_sha1) *arg, char *src, char *dst);
LOCAL bool hmac_sha1_base64_encode(CLASS(hmac_sha1) *arg, const char *src, size_t src_size, char *dst);


LOCAL bool delete_hmac_sha1(CLASS(hmac_sha1) *arg);

/******************************************************************************
 * FunctionName : init_hmac_sha1
 * Description	: internal used to initiate object 
 * Parameters	: CLASS(hmac_sha1) *arg point to the struct need initailizing 
 *					
 * Returns		: TRUE   : initialzie successfully
 *				  FALSE  : initilize failedly 	
*******************************************************************************/
#if 1/*for debugging purpose*/
bool ICACHE_FLASH_ATTR
init_hmac_sha1(CLASS(hmac_sha1) *arg)
{
	assert(NULL != arg);
	

	struct enpryption_information *enpryption_data = (struct enpryption_information*)os_malloc(sizeof(struct enpryption_information));
	assert(NULL != enpryption_data);
	/*clear memory first*/
	os_memset(enpryption_data,  sizeof(struct enpryption_information), 0);
	/*assign private enprytion data to object*/
	arg->user_data = (void*) enpryption_data;


	/*assign initiatial function to struct member init*/
	arg->init =    init_hmac_sha1;
	arg->set_key = hmac_sha1_set_key;
	arg->set_text = hmac_sha1_set_text;
	arg->process = hmac_sha1_process;

	arg->base64_encode = hmac_sha1_base64_encode;
	arg->base64_decode = hmac_sha1_base64_decode;
	arg->de_init = delete_hmac_sha1;
	return TRUE;
}
#endif
/******************************************************************************
 * FunctionName : init_hmac_sha1
 * Description	: internal used to initiate object 
 * Parameters	: CLASS(hmac_sha1) *arg point to the struct need initailizing 
 *					
 * Returns		: TRUE   : initialzie successfully
 *				  FALSE  : initilize failedly 	
*******************************************************************************/
#if 1/*for debugging purpose*/
LOCAL bool ICACHE_FLASH_ATTR
delete_hmac_sha1(CLASS(hmac_sha1) *arg)
{

	assert(NULL != arg);
	
	struct enpryption_information *enpryption_data = ((struct enpryption_information*)(arg->user_data));
	/*free private data*/
	if (NULL != enpryption_data){
		os_free(enpryption_data);
	}

	/*delete object*/
	os_free(arg);
	
	return TRUE;
}
#endif

/******************************************************************************
 * FunctionName : hmac_sha1_set_key
 * Description	: internal used to initiate object 
 * Parameters	: CLASS(hmac_sha1) *arg point to the struct need initailizing 
 *					
 * Returns		: TRUE   : initialzie successfully
 *				  FALSE  : initilize failedly 	
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
hmac_sha1_set_key (CLASS(hmac_sha1) *arg, char *key, size_t size)
{
	/*check parameter object first*/
	assert(NULL != arg);

	/*check user private data pointer thridly*/
	assert(NULL != (arg->user_data));
	
	/*check parameter key pointer secondly*/
	assert(NULL != (void*)key);

	os_memcpy((void*)(((struct enpryption_information*)(arg->user_data))->key), (void*)key, size);
	
	((struct enpryption_information *)(arg->user_data))->key_lenth = size;

	return TRUE;
}

/******************************************************************************
 * FunctionName : hmac_sha1_set_text
 * Description	: internal used to move text content from user buffer to internal buffer 
 * Parameters	: CLASS(hmac_sha1) *arg point to the struct need initailizing 
 *					
 * Returns		: TRUE   : initialzie successfully
 *				  FALSE  : initilize failedly 	
*******************************************************************************/
#if 1
LOCAL bool ICACHE_FLASH_ATTR
hmac_sha1_set_text (CLASS(hmac_sha1) *arg, char *text, size_t size)
{
	/*check parameter object first*/
	assert(NULL != arg);

	/*check user private data pointer thridly*/
	assert(NULL != (arg->user_data));
	
	/*check parameter key pointer secondly*/
	assert(NULL != text);

	os_memcpy(((struct enpryption_information*)(arg->user_data))->text, text, size);
	
	((struct enpryption_information*)(arg->user_data))->text_lenth = size;

	return TRUE;
}
#endif


/******************************************************************************
 * FunctionName : hmac_sha1_process
 * Description	: internal used to enpryt text
 * Parameters	: CLASS(hmac_sha1) *arg point to the struct need initailizing 
 *					
 * Returns		: TRUE   : initialzie successfully
 *				  FALSE  : initilize failedly 	
*******************************************************************************/
#if 1
LOCAL bool ICACHE_FLASH_ATTR
hmac_sha1_process  (CLASS(hmac_sha1) *arg, char *output_buffer, size_t *size)
{

	struct enpryption_information *enpryption_data = ((struct enpryption_information*)(arg->user_data));
	/*check parameter object first*/
	assert(NULL != arg && NULL != (arg->user_data));
	
	/*check parameter key pointer secondly*/
	assert(NULL != output_buffer && NULL != size && NULL != enpryption_data);

	*size = 20;
	/*enpryption the text with the key*/
	return hmac_sha1_process_internal(enpryption_data->text, enpryption_data->text_lenth, enpryption_data->key, enpryption_data->key_lenth, output_buffer);

}
#endif



/******************************************************************************
 * FunctionName : hmac_sha1_process
 * Description  : user interface to enpryt data 
 * Parameters   : unsigned char *text  : buffer pointer pointed to the data needing for encryption
 *				  int text_len,		   : lenth of buferr passed to this interface
 * 				  unsigned char *key,  : encryption key poniter
 *				  int key_len, 		   : encryption key lenth
 *				  unsigned char *digest: buffer where encryption result stored
 * Returns      : NONE
*******************************************************************************/

	
LOCAL bool ICACHE_FLASH_ATTR
hmac_sha1_process_internal(unsigned char *text, int text_len, unsigned char *key, int key_len, unsigned char *digest)
{


    unsigned char  m_ipad[64] = {0};
	unsigned char  m_opad[64] = {0};
	char  szReport[SHA1_BLOCK_SIZE] = {0};
	char  SHA1_Key[SHA1_BLOCK_SIZE] = {0};
	char  AppendBuf1[sizeof(m_ipad) + 128] = {0};
	char  AppendBuf2[sizeof(m_ipad) + 128] ={0};
	int i=0, j=0;
	/*check parameter passed from user*/
	if (text == NULL){
		return FALSE;
	}
	
#if 0
	/*os_malloc coresponed area for temparory store*/
	AppendBuf1 = (char*)os_malloc(sizeof(m_ipad) + text_len);
	if (AppendBuf1 == NULL){
		return FALSE;
	
	}

	/*os_malloc coresponed area for temparory store*/
	AppendBuf2 = (char*)os_malloc(sizeof(m_opad) + SHA1_DIGEST_LENGTH);
	if (AppendBuf2 == NULL){

		os_free(AppendBuf1);
		return FALSE;
	
	}

#endif
	
	/*do some preparation for running of csha1 and reset parameters  to defaul*/
 	csha1_init();

	os_memset(SHA1_Key, 0, SHA1_BLOCK_SIZE);

	/* repeated 64 times for values in ipad and opad */
	os_memset(m_ipad, 0x36, sizeof(m_ipad));
	os_memset(m_opad, 0x5c, sizeof(m_opad));

	/* STEP 1 */
	if (key_len > SHA1_BLOCK_SIZE)
	{
		csha1_reset();
		csha1_update((uint8 *)key, key_len);
		csha1_final();

		csha1_getHash((uint8 *)SHA1_Key);
	}
	else
		os_memcpy(SHA1_Key, key, key_len);

	/* STEP 2 */
	for (i=0; i < sizeof(m_ipad); i++)
	{
		m_ipad[i] ^= SHA1_Key[i];		
	}

	/* STEP 3 */
	os_memcpy(AppendBuf1, m_ipad, sizeof(m_ipad));
	os_memcpy(AppendBuf1 + sizeof(m_ipad), text, text_len);


	/* STEP 4 */
	csha1_reset();
	csha1_update((uint8 *)AppendBuf1, sizeof(m_ipad) + text_len);
	csha1_final();

	csha1_getHash((uint8 *)szReport);


	/* STEP 5 */
	for (j=0; j<sizeof(m_opad); j++)
	{
		m_opad[j] ^= SHA1_Key[j];
	}
	

	/* STEP 6 */
	os_memcpy(AppendBuf2, m_opad, sizeof(m_opad));
	os_memcpy(AppendBuf2 + sizeof(m_opad), szReport, SHA1_DIGEST_LENGTH);

	/*STEP 7 */
	csha1_reset();
	csha1_update((uint8 *)AppendBuf2, sizeof(m_opad) + SHA1_DIGEST_LENGTH);
	csha1_final();

	csha1_getHash((uint8 *)digest);
	
#if 0

/*os_free memory from heap*/
	os_free(AppendBuf1);
	os_free(AppendBuf2);
#endif	
	return TRUE;
}

/******************************************************************************
 * FunctionName : base64_decode
 * Description  : user interface to enpryt data 
 * Parameters   : unsigned char *text  : buffer pointer pointed to the data needing for encryption
 *				  int text_len,		   : lenth of buferr passed to this interface
 * 				  unsigned char *key,  : encryption key poniter
 *				  int key_len, 		   : encryption key lenth
 *				  unsigned char *digest: buffer where encryption result stored
 * Returns      : NONE
*******************************************************************************/

LOCAL bool ICACHE_FLASH_ATTR
hmac_sha1_base64_decode(CLASS(hmac_sha1) *arg, char *src, char *dst)
{
    char *q = (char*)os_malloc(strlen(src)+1);
    char *p = dst;
    char *temp = q;
    char *s = src;
    int len = strlen(src),i;
    os_memset(q, 0, strlen(src)+1);
    while(*s)
    {
        if(*s>='A'&&*s<='Z') *temp=*s-'A';
        else if(*s>='a'&&*s<='z') *temp=*s-'a'+26;
        else if(*s>='0'&&*s<='9') *temp=*s-'0'+52;
        else if(*s=='+') *temp=62;
        else if(*s=='/') *temp=63;
        else if(*s=='=') *temp=-1;
        else
        {
            CLING_DEBUG("\n%c:Not a valid base64 string\n",*s);
            return FALSE;
        }
        ++s;
        ++temp;
    }
    for(i=0;i<len-4;i+=4)
    {
        *p++=(*(q+i)<<2)+(*(q+i+1)>>4);
        *p++=(*(q+i+1)<<4)+(*(q+i+2)>>2);
        *p++=(*(q+i+2)<<6)+(*(q+i+3));
    }
    if(*(q+i+3)!=-1)
    {
        *p++=(*(q+i)<<2)+(*(q+i+1)>>4);
        *p++=(*(q+i+1)<<4)+(*(q+i+2)>>2);
        *p++=(*(q+i+2)<<6)+*(q+i+3);
    }
    else if(*(q+i+2)!=-1)
    {
        *p++=(*(q+i)<<2)+(*(q+i+1)>>4);
        *p++=(*(q+i+1)<<4)+(*(q+i+2)>>2);
        *p++=(*(q+i+2)<<6);
    }
    else if(*(q+i+1)!=-1)
    {
        *p++=(*(q+i)<<2)+(*(q+i+1)>>4);
        *p++=(*(q+i+1)<<4);
    }
    else 
    {
        CLING_DEBUG("Not a valid base64 string\n");
        return FALSE;
    }
    *p=0;
    os_free(q);
}

/******************************************************************************
 * FunctionName : base64_encode
 * Description  : user interface to encode data
 * Parameters   : unsigned char *text  : buffer pointer pointed to the data needing for encryption
 *				  int text_len,		   : lenth of buferr passed to this interface
 * 				  unsigned char *key,  : encryption key poniter
 *				  int key_len, 		   : encryption key lenth
 *				  unsigned char *digest: buffer where encryption result stored
 * Returns      : NONE
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
hmac_sha1_base64_encode(CLASS(hmac_sha1) *arg, const char *src, size_t src_size, char *dst)
{
    int i = 0;
    char *p = dst;
    int d = src_size-3;
    //for(i=0;i<strlen(src)-3;i+=3) ;if (strlen(src)-3)<0 there is a buf
	
    for(i=0;i<=d;i+=3)
    {
        *p++=Base64[((*(src+i))>>2)&0x3f];
        *p++=Base64[(((*(src+i))&0x3)<<4)+((*(src+i+1))>>4)];
        *p++=Base64[((*(src+i+1)&0xf)<<2)+((*(src+i+2))>>6)];
        *p++=Base64[(*(src+i+2))&0x3f];
    }
    if((src_size - i) == 1)
    {
        *p++=Base64[((*(src+i))>>2)&0x3f];
        *p++=Base64[((*(src+i))&0x3)<<4];
        *p++='=';
        *p++='=';
    }
    if((src_size - i) == 2)
    {
        *p++=Base64[((*(src+i))>>2)&0x3f];
        *p++=Base64[(((*(src+i))&0x3)<<4)+((*(src+i+1))>>4)];
        *p++=Base64[((*(src+i+1)&0xf)<<2)];
        *p++='=';
    }
    *p='\0';
}





