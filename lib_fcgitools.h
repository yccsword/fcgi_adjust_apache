#ifndef _LIB_FCGITOOLS_H_
#define _LIB_FCGITOOLS_H_

#include "os.h"
#include <sys/time.h>
#include "fcgi_stdio.h"
//#define NDEBUG
#include <assert.h>
#include "multipart_parser.h"
#include "pthread.h"
#include <malloc.h>
#include "md5.h"

#define MACRO_STR(x) {x, #x}  

#ifndef _MACRO_STR_T
#define _MACRO_STR_T
typedef struct _macro_str {	
   int id;	
   char *name;	
}MACRO_STR_T;  
#endif
 
typedef enum _http_code{  
	/*********************************** HTTP Codes *******************************/
	/*
	Standard HTTP/1.1 status codes
	*/
	HTTP_CODE_CONTINUE                  =100,     /**< Continue with request, only partial content transmitted */
	HTTP_CODE_OK                        =200,     /**< The request completed successfully */
	HTTP_CODE_CREATED                   =201,     /**< The request has completed and a new resource was created */
	HTTP_CODE_ACCEPTED                  =202,     /**< The request has been accepted and processing is continuing */
	HTTP_CODE_NOT_AUTHORITATIVE         =203,     /**< The request has completed but content may be from another source */
	HTTP_CODE_NO_CONTENT                =204,     /**< The request has completed and there is no response to send */
	HTTP_CODE_RESET                     =205,     /**< The request has completed with no content. Client must reset view */
	HTTP_CODE_PARTIAL                   =206,     /**< The request has completed and is returning partial content */
	HTTP_CODE_MOVED_PERMANENTLY         =301,     /**< The requested URI has moved permanently to a new location */
	HTTP_CODE_MOVED_TEMPORARILY         =302,     /**< The URI has moved temporarily to a new location */
	HTTP_CODE_SEE_OTHER                 =303,     /**< The requested URI can be found at another URI location */
	HTTP_CODE_NOT_MODIFIED              =304,     /**< The requested resource has changed since the last request */
	HTTP_CODE_USE_PROXY                 =305,     /**< The requested resource must be accessed via the location proxy */
	HTTP_CODE_TEMPORARY_REDIRECT        =307,     /**< The request should be repeated at another URI location */
	HTTP_CODE_BAD_REQUEST               =400,     /**< The request is malformed */
	HTTP_CODE_UNAUTHORIZED              =401,     /**< Authentication for the request has failed */
	HTTP_CODE_PAYMENT_REQUIRED          =402,     /**< Reserved for future use */
	HTTP_CODE_FORBIDDEN                 =403,     /**< The request was legal, but the server refuses to process */
	HTTP_CODE_NOT_FOUND                 =404,     /**< The requested resource was not found */
	HTTP_CODE_BAD_METHOD                =405,     /**< The request HTTP method was not supported by the resource */
	HTTP_CODE_NOT_ACCEPTABLE            =406,     /**< The requested resource cannot generate the required content */
	HTTP_CODE_REQUEST_TIMEOUT           =408,    /**< The server timed out waiting for the request to complete */
	HTTP_CODE_CONFLICT                  =409,    /**< The request had a conflict in the request headers and URI */
	HTTP_CODE_GONE                      =410,    /**< The requested resource is no longer available*/
	HTTP_CODE_LENGTH_REQUIRED           =411,    /**< The request did not specify a required content length*/
	HTTP_CODE_PRECOND_FAILED            =412,    /**< The server cannot satisfy one of the request preconditions */
	HTTP_CODE_REQUEST_TOO_LARGE         =413,    /**< The request is too large for the server to process */
	HTTP_CODE_REQUEST_URL_TOO_LARGE     =414,    /**< The request URI is too long for the server to process */
	HTTP_CODE_UNSUPPORTED_MEDIA_TYPE    =415,    /**< The request media type is not supported by the server or resource */
	HTTP_CODE_RANGE_NOT_SATISFIABLE     =416,    /**< The request content range does not exist for the resource */
	HTTP_CODE_EXPECTATION_FAILED        =417,    /**< The server cannot satisfy the Expect header requirements */
	HTTP_CODE_NO_RESPONSE               =444,    /**< The connection was closed with no response to the client */
	HTTP_CODE_INTERNAL_SERVER_ERROR     =500,    /**< Server processing or configuration error. No response generated */
	HTTP_CODE_NOT_IMPLEMENTED           =501,    /**< The server does not recognize the request or method */
	HTTP_CODE_BAD_GATEWAY               =502,    /**< The server cannot act as a gateway for the given request */
	HTTP_CODE_SERVICE_UNAVAILABLE       =503,    /**< The server is currently unavailable or overloaded */
	HTTP_CODE_GATEWAY_TIMEOUT           =504,    /**< The server gateway timed out waiting for the upstream server */
	HTTP_CODE_BAD_VERSION               =505,    /**< The server does not support the HTTP protocol version */
	HTTP_CODE_INSUFFICIENT_STORAGE      =507,    /**< The server has insufficient storage to complete the request */

	/*
	Proprietary HTTP status codes
	*/
	HTTP_CODE_START_LOCAL_ERRORS        =550,
	HTTP_CODE_COMMS_ERROR               =550,    /**< The server had a communicationss error responding to the client */
	HTTP_CODE_END
}HTTP_CODE;	
 
char * lib_fcgitools_get_macro_name(MACRO_STR_T* table, int id);

#define MAX_QUERY_COUNT 32
#define MAX_SESSION_LEN 384 /*128 * 3*/
#define MAX_ENCODE_LEN 129 /*(MAX_SESSION_LEN / 3) + 1*/
#define MAXL_BASE64CODE    1024
#define UPLOAD_FILE_PREFIX "/tmp/"
#define UPLOAD_FILE_PREFIX_LEN 6 /*strlen(UPLOAD_FILE_PREFIX) + 1*/
#define MAX_UPLOADFILE_COUNT 8
#define MAX_UPLOADFILE_NAME_LEN 67 /*64 + 3*/

typedef enum http_env_list 
{
	REQUEST_METHOD = 0,
	QUERY_STRING,
	HTTP_SESSION,
	CONTENT_LENGTH,
	CONTENT_TYPE,
	REMOTE_ADDR,
	SERVER_ADDR,
	SERVER_PORT,
	HTTP_HOST,
	HTTP_ENV_END
}
HTTP_ENV_LIST;

typedef struct {
	char *  QueryName;
	char *  QueryValue;
} Query;

typedef struct Webs{
	char			*method;			/**< HTTP request method */
	int        		files;              /**< Uploaded files */
	char			*content_type;
	char			*query;
	int				content_length;
	char			*content;
	char			ipaddr[64];			/**< Connecting ipaddress */
	char            ifaddr[64];  		/**< Local interface ipaddress */
	int 			port;
	char            host[64];  			/**< Requested host */
	int reserver;
} Webs;

extern Query gQuery[MAX_QUERY_COUNT];
extern int QueryCount;
extern char Null_String;
extern char X_Replace_Session[MAX_SESSION_LEN];
extern int HttpStatus;

#define CLEAN_QUERY 1
#define KEEP_QUERY 0

void split(char *src, const char *separator, char **dest, int *num);
#define ParseQuery(query_string) _ParseQuery(query_string, CLEAN_QUERY)
#define ParseQuery_Add(query_string) _ParseQuery(query_string, KEEP_QUERY)
char * GetQueryValue(char * Key);
char * GetQueryString(void);
void FreeQueryString(char * querystring);
char * urldecode(char *p);
char * urlencode(char const *s);
char * base64_encode(const char* str);
char * base64_decode(const char* str);
int findstr(char* src, char* s);
void strToUpper(char * str);
void StrToHex(unsigned char *pbDest, unsigned char *pbSrc, int nLen);
void HexToStr(unsigned char *pbDest, unsigned char *pbSrc, int nLen);
Webs * FCGI_InitWp(void);
void FCGI_FreeWp(Webs * wp);
char * FCGI_GetPostFormData_clientFileName(int fileindex);
char * FCGI_GetPostFormData_FileName(int fileindex);
unsigned int FCGI_GetPostFormData_FileLength(int fileindex);
char * FCGI_GetPostFormData_FileContentType(int fileindex);


#define FCGI_ErrLog(...)                    fprintf(stderr,"%s:%d ",__func__,__LINE__);fprintf(stderr,__VA_ARGS__);
#define FCGI_PrintTest(TestMessage) 		_FCGI_PrintTest(TestMessage, __func__, __LINE__)
#define FCGI_PrintWeb(Status, Content_type, RedirectUrlorMessage) _FCGI_PrintWeb(Status, Content_type, RedirectUrlorMessage, __func__, __LINE__)

#define MAX_APACHE_MSG_LEN 1024 /*1KB*/
#define MAX_APACHE_TEMPMSG_LEN 511 /*512 - 1,空出两个\0的位置*/
#define MAX_MSG_BLOCK 1024 /*1024 * 1KB = 1MB*/

#ifdef GLOBAL_VARIABLES_HERE
char szMsg[MAX_APACHE_MSG_LEN];
int msg_block = 1;
char * pMsg = szMsg;
#else
extern char szMsg[MAX_APACHE_MSG_LEN];
extern int msg_block;
extern char * pMsg;
#endif
#define Message pMsg

#define WEBS_SESSION_USERNAME "creatcomm-user"
#define WEBS_SESSION_USERID "creatcomm-userid"
#define WEBS_SESSION_ABILITY "creatcomm-userability"

#define websGetVar(wp, name, defaultValue) GetQueryValue(name)
#define websGetSessionVar(wp, name, defaultValue) GetQueryValue(name)
#define websSetSessionVar(wp, name, value) FCGI_SetSession(name, value)

#define MSG_LEN ((msg_block == 1) ? MAX_APACHE_MSG_LEN : \
				(((msg_block * MAX_APACHE_MSG_LEN) < (malloc_usable_size(pMsg) - 8)) ? \
				(msg_block * MAX_APACHE_MSG_LEN) : (malloc_usable_size(pMsg) - 8)))
inline void ZeroMsg(void);
inline int ExternMsgMem(void);

#if 1
#define websWrite(wp, ...) ({char tempmsg[MAX_APACHE_TEMPMSG_LEN] = {0};\
							int oldlen = 0,addlen = 0;\
							int ret = 0;\
							int msg_len = MSG_LEN;\
							ret = snprintf(tempmsg, MAX_APACHE_TEMPMSG_LEN, __VA_ARGS__);\
							oldlen = strlen(pMsg);addlen = strlen(tempmsg);\
							if (oldlen + addlen >= msg_len && ExternMsgMem() != 0) \
							{FCGI_ErrLog("%s is too long for fcgi msg!max:%d has:%d cur:%d\n",\
								tempmsg, msg_len, oldlen, addlen);\
							addlen = msg_len - oldlen - 1;ret = -1;}\
							memcpy(&pMsg[oldlen], tempmsg, addlen);\
							pMsg[oldlen + addlen + 1] = '\0';\
							ret;})
#else
#define websWrite(wp, ...) FCGI_websWrite(Message, __VA_ARGS__)
#endif

#define websRedirect(wp,url) FCGI_PrintWeb(302, "text/html", url);

#define websGetDocuments() "apache_fcgi"
#define websSetStatus(wp, code) HttpStatus = code
#define websWriteBlock(wp, buf, size) websWrite(wp, "%s", buf)
#define websDone(wp)

#define WP_HTML_HEAD_WP(wp) 
#define WP_HTML_END_WP(wp) 
#define WP_RETURN_EMPTY(wp) 
#define WP_RETURN_RSP(wp) websWrite(wp, "OK")
#define WP_RETURN_DATA_RSP(wp, data) websWrite(wp, "%s", data)
#define WP_RETURN_DATA_NUMBER_RSP(wp, data) websWrite(wp, "%d", data)

//FCGI no need pthread disable pthread(func must be void * func (void * args))
#ifdef GLOBAL_VARIABLES_HERE
char *thread_list[] = {
	"reboot_thread",
	"upgradeACImage_thread",
	"recoveryAC_thread",
	"\0"
};
#else
extern char *thread_list[];
#endif	
#define no_thread(thread_name) ({int i = 0;\
								while(thread_list[i][0] != '\0')\
								{\
									if (0 == strcmp(thread_list[i], thread_name)) {i = 0; break;}\
									i++;\
								}i;})
#define pthread_create(A, B, func, args) no_thread(#func)?func(args):pthread_create(A, B, func, args)

#endif
