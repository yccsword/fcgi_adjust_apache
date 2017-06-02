#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lib_fcgitools.h"
#include <stdarg.h>
#include <errno.h>  

Query gQuery[MAX_QUERY_COUNT] = {{0}};
int QueryCount = 0;
char Null_String = '\0';
char X_Replace_Session[MAX_SESSION_LEN] = {0};
multipart_parser_settings callbacks;
multipart_parser* parser = NULL;
char uploadfile_name[MAX_UPLOADFILE_COUNT][MAX_UPLOADFILE_NAME_LEN] = {{0}};
char uploadfile_full_name[MAX_UPLOADFILE_COUNT][MAX_UPLOADFILE_NAME_LEN + UPLOAD_FILE_PREFIX_LEN] = {{0}};
char uploadfile_content_type[MAX_UPLOADFILE_COUNT][MAX_UPLOADFILE_NAME_LEN] = {{0}};
unsigned int uploadfile_size[MAX_UPLOADFILE_COUNT] = {0};
int uploadfile_count = 0;

Webs wp = {0};

void split(char *src, const char *separator, char **dest, int *num)
{
    char *pNext;
    int count = 0;
    
    if (src == NULL || strlen(src) == 0) return;
    if (separator == NULL || strlen(separator) == 0) return; 

    pNext = strsep(&src,separator);
    
    while(pNext != NULL)
    {
        *dest++ = pNext;
        ++count;
        pNext = strsep(&src,separator);
    }

    *num = count;
}

void trim(char* s, char c)
{
    char *t  = s;
    while (*s == c){s++;};
    if (*s)
    {
        char* t1 = s;
        while (*s){s++;};
        s--;
        while (*s == c){s--;};
        while (t1 <= s)
        {
            *(t++) = *(t1++);
        }
    }
    *t = 0;
}

void _ParseQuery(char * query_string, int NeedClean)
{ 
	if (NeedClean == CLEAN_QUERY)
	{
		QueryCount = 0;
		memset(gQuery, 0, sizeof(gQuery));
	}
	int i = 0;
	char * tempquery[2] = {0};
	int valuecount = 0;
	int curQueryCount = 0;
	int lastQueryCount = QueryCount;
	char * QueryChar[MAX_QUERY_COUNT] = {0};
	split(query_string, "&", QueryChar, &curQueryCount);
	if (MAX_QUERY_COUNT < (lastQueryCount + curQueryCount))
	{
		FCGI_ErrLog("query:%s has too many element,you will lost some data,max:%d last:%d cur:%d \n", 
					query_string, MAX_QUERY_COUNT, lastQueryCount, curQueryCount);
		curQueryCount = MAX_QUERY_COUNT - lastQueryCount;
	}
	QueryCount = lastQueryCount + curQueryCount;
	for (i = 0; i < curQueryCount ; i++)
	{
		split(QueryChar[i], "=", tempquery, &valuecount);
		gQuery[lastQueryCount + i].QueryName = tempquery[0];
		gQuery[lastQueryCount + i].QueryValue = tempquery[1];	
	}
}

char * GetQueryValue(char * Key)
{
	int i = 0;
	for (i = 0; i < QueryCount; i++)
	{
		if (strcmp(gQuery[i].QueryName, Key) == 0)
		{
			return urldecode(gQuery[i].QueryValue);
		}
	}
	return &Null_String;
}

char * g_querystring = NULL;
char * GetQueryString(void)
{
	char buf[256] = {0};
	char *querystring = NULL;
	char *old_querystring = NULL;
	int querycount = 0;
	memset(buf, 0, sizeof(buf));
	FreeQueryString(NULL);
	while(NULL != FCGI_fgets(buf, sizeof(buf), FCGI_stdin))
	{  
		querycount++;
		old_querystring = querystring;
		querystring = (char *)realloc(old_querystring, querycount * sizeof(buf));
		if (querycount == 1) {memset(querystring, 0, querycount * sizeof(buf));}
		if (querystring == NULL)
		{
			free(old_querystring);
			old_querystring = NULL;
			return &Null_String;
		}
		memcpy(querystring + (querycount - 1) * (sizeof(buf) - 1), buf, sizeof(buf));
	}
	g_querystring = querystring;
	return querystring;
}

void FreeQueryString(char * querystring)
{
	if (querystring != NULL)
	{
		free(querystring);
		querystring = NULL;
		g_querystring = NULL;
	}
	else if (g_querystring != NULL)
	{
		free(g_querystring);
		g_querystring = NULL;
	}
}

int DeleteFile(char * filepath)
{
	if (filepath != NULL)
	{
		if(remove(filepath) == 0)
		{
        	FCGI_ErrLog("Removed %s.", filepath);
			return 0;
		}
    	else
    	{
        	perror("remove");
			return 1;
    	}
	}
	else
	{
		int i = 0;
		int filepath_prefix_len = strlen(UPLOAD_FILE_PREFIX);
		int uploadfile_name_len = 0;

		filepath = alloca(MAX_UPLOADFILE_NAME_LEN + filepath_prefix_len);
		if (filepath == NULL) {FCGI_ErrLog("DeleteFile cmd alloca error!\n");return 2;}
		memset(filepath, 0, MAX_UPLOADFILE_NAME_LEN + filepath_prefix_len);
		memcpy(filepath, UPLOAD_FILE_PREFIX, filepath_prefix_len);
	
		for (i = 0; i < uploadfile_count; i++)
		{
			uploadfile_name_len = strlen(uploadfile_name[i]);
			if (uploadfile_name_len == 0) {FCGI_ErrLog("all temp file has delete!\n");return 0;}
			memset(filepath + filepath_prefix_len, 0, MAX_UPLOADFILE_NAME_LEN);	
			memcpy(filepath + filepath_prefix_len, uploadfile_name[i], uploadfile_name_len);
			remove(filepath);
		}
	}
	return 0;
}

char *filepath = NULL;
char *post_form_data = NULL;
unsigned int post_form_data_len = 0;
int isContentType = 0;
int start_data_write(multipart_parser* p)
{return 0;}

int end_data_write(multipart_parser* p)
{
	if (filepath != NULL)
	{
		free(filepath);
		filepath = NULL;
		uploadfile_count++;
	}
	return 0;
}

int parse_end (multipart_parser* p)
{
	if (filepath != NULL)
	{
		free(filepath);
		filepath = NULL;
	}
	return 0;
}

int read_header_name(multipart_parser* p, const char *at, size_t length)
{
	//FCGI_ErrLog("%.*s: ", length, at);
	char * pos = NULL;
	if ((pos = strstr(at, "Content-Type")) != NULL && (pos - at) <= length)
	{
		isContentType = 1;
	}
	return 0;
}

int read_header_value(multipart_parser* p, const char *at, size_t length)
{
	char * filename = strstr(at, "filename=");//获取filename位置的指针地址
	unsigned int prefix_len = (filename > at) ? (filename - at) : (at - filename);//获取content-head中filename前的数据长度
	int filename_len = (length >= prefix_len + 9) ? (length - prefix_len - 9) : -1;//获取filename长度
	char * form_dataname = NULL;
	
	//FCGI_ErrLog("%.*s \n", length, at);
	//由于at指针携带是整个post content的内容，如果filedata不是在这个head下，需要转form-data处理
	if (filename != NULL && filename_len >= 0)
	{
		if (uploadfile_count >= MAX_UPLOADFILE_COUNT)
		{
			FCGI_ErrLog("Max upload file onetime is %d,please upload %s next time.\n", MAX_UPLOADFILE_COUNT, filename);
			return 0;
		}
		if (filename_len >= MAX_UPLOADFILE_NAME_LEN)
		{
			FCGI_ErrLog("Max upload filename len is %d,file ", MAX_UPLOADFILE_NAME_LEN - 3);
			fwrite((void *)filename + 9, filename_len, 1, stderr);
			fprintf(stderr," name len is %d too long.\n", filename_len - 2);
			return 0;
		}
		memcpy(uploadfile_name[uploadfile_count], filename + 9, filename_len);
		trim(uploadfile_name[uploadfile_count], '\"');
		//if (uploadfile_name[uploadfile_count] == NULL) {FCGI_ErrLog("uploadfile_name[%d] is NULL!\n", uploadfile_count);return 0;}
		int uploadfile_name_len = strlen(uploadfile_name[uploadfile_count]);
		if (uploadfile_name_len == 0) {FCGI_ErrLog("uploadfile_name_len is 0!\n");return 0;}
		int filepath_prefix_len = strlen(UPLOAD_FILE_PREFIX);
		filepath = calloc(1, filepath_prefix_len + MAX_UPLOADFILE_NAME_LEN);
		if (filepath == NULL) {FCGI_ErrLog("filepath calloc error!\n");return 0;}
		memcpy(filepath, UPLOAD_FILE_PREFIX, filepath_prefix_len);
		memcpy(filepath + filepath_prefix_len, uploadfile_name[uploadfile_count], uploadfile_name_len);
		filepath[filepath_prefix_len + uploadfile_name_len] = '\0';
	}
	//form-data只能作为该条记录的开始
	else if (strstr(at, "form-data;") != NULL && (form_dataname = strstr(at, "name=")) != NULL && (form_dataname - at) <= length)
	{
		unsigned int form_dataname_prefix_len = form_dataname - at;
		int form_dataname_len = length - form_dataname_prefix_len - 5;
		char * last_post_form_data = post_form_data;

		char * real_form_dataname = alloca(form_dataname_len + 1);
		if (real_form_dataname == NULL) {FCGI_ErrLog("real_form_dataname alloca error!drop %.*s data.\n", length, at);return 0;}
		memcpy(real_form_dataname, form_dataname + 5, form_dataname_len);
		real_form_dataname[form_dataname_len] = '\0';
		trim(real_form_dataname, '\"');
		int real_form_dataname_len = strlen(real_form_dataname);
		unsigned int new_post_form_data_len = post_form_data_len + real_form_dataname_len + 2;
		
		post_form_data = (char *)realloc(last_post_form_data, new_post_form_data_len);
		if (post_form_data == NULL)
		{
			post_form_data = last_post_form_data;
			FCGI_ErrLog("realloc post_form_data error! drop %.*s data.\n", length, at);
			return 0;
		}

		//FCGI_ErrLog("real_form_dataname:%s ", real_form_dataname);
		
		post_form_data[(post_form_data_len == 0) ? 0 : (post_form_data_len - 1)] = '&';
		memcpy(post_form_data + post_form_data_len, real_form_dataname, real_form_dataname_len);
		post_form_data[new_post_form_data_len - 2] = '=';
		post_form_data[new_post_form_data_len - 1] = '\0';
		post_form_data_len = new_post_form_data_len;
		//FCGI_ErrLog("%.*s: ", length, at);
		//FCGI_ErrLog("post_form_data:%s post_form_data_len:%d", post_form_data, post_form_data_len);
	}
	else if (isContentType == 1)
	{
		memcpy(uploadfile_content_type[uploadfile_count], at, length);//获取文件的content_type
		//FCGI_ErrLog("uploadfile_content_type:%s\n", uploadfile_content_type[uploadfile_count]);
		isContentType = 0;
	}
	return 0;
}

int write_part(multipart_parser* p, const char *at, size_t length)
{
	if (filepath == NULL) 
	{
		if (post_form_data != NULL)
		{
			if (post_form_data[post_form_data_len - 2] == '=')
			{
				char * last_post_form_data = post_form_data;
				unsigned int new_post_form_data_len = post_form_data_len + length;
				post_form_data = (char *)realloc(last_post_form_data, new_post_form_data_len);
				if (post_form_data == NULL)
				{
					post_form_data = last_post_form_data;
					FCGI_ErrLog("realloc post_form_data error! drop %.*s data part.\n", length, at);
					return 0;
				}
				memcpy(post_form_data + post_form_data_len - 1, at, length);
				post_form_data[new_post_form_data_len - 1] = '\0';
				post_form_data_len = new_post_form_data_len;
				//FCGI_ErrLog("post_form_data:%s post_form_data_len:%d", post_form_data, post_form_data_len);
			}
			//FCGI_ErrLog("%.*s \n", length, at);
		}
		else
		{
			FCGI_ErrLog("this part not a file and not a form data, please check!\n");
		}
		return 0;
	}
	if (uploadfile_count >= MAX_UPLOADFILE_COUNT)
	{
		FCGI_ErrLog("Max upload file onetime is %d,this file will drop.\n", MAX_UPLOADFILE_COUNT);
		return 0;
	}
	FILE *fp = NULL;
	if ((fp = fopen(filepath, "ab+")) == NULL)
	{
		FCGI_ErrLog("Write uploadfile open fp fail!\n");
		return 0;
	}
	fwrite((void *)at, length, 1, fp);
	uploadfile_size[uploadfile_count] += length;//累计文件长度
	fflush(fp);  
	fclose(fp);
	return 0;
}

Webs * FCGI_InitWp(void)
{
	FCGI_FreeWp(&wp);
	QueryCount = 0;
	memset(gQuery, 0, sizeof(gQuery));
	uploadfile_count = 0;
	memset(uploadfile_name, 0, sizeof(uploadfile_name));
	memset(uploadfile_full_name, 0, sizeof(uploadfile_full_name));
	memset(uploadfile_content_type, 0, sizeof(uploadfile_content_type));
	memset(uploadfile_size, 0, sizeof(uploadfile_size));
	char *http_env[HTTP_ENV_END] = {0};
	int boundlen = 0;
	char *bound = NULL;/*${bound}*/
	char *__bound = NULL;/*--${bound}*/
	int multipart_parser_execute_len = 0;
	http_env[REQUEST_METHOD] = getenv("REQUEST_METHOD");
	http_env[QUERY_STRING] = getenv("QUERY_STRING");
	http_env[HTTP_SESSION] = getenv("HTTP_SESSION");
	http_env[REMOTE_ADDR] = getenv("REMOTE_ADDR");
	http_env[SERVER_ADDR] = getenv("SERVER_ADDR");
	
	wp.method = http_env[REQUEST_METHOD];
	wp.query_string = strdup(http_env[QUERY_STRING]);
	memcpy(wp.ipaddr, http_env[REMOTE_ADDR], sizeof(wp.ipaddr));
	memcpy(wp.ifaddr, http_env[SERVER_ADDR], sizeof(wp.ifaddr));

	if ((http_env[QUERY_STRING] != NULL) && (strlen(http_env[QUERY_STRING]) != 0))
	{
		ParseQuery_Add(http_env[QUERY_STRING]);
	}

	if ((http_env[HTTP_SESSION] != NULL) && (strlen(http_env[HTTP_SESSION]) != 0))
	{
		ParseQuery_Add(http_env[HTTP_SESSION]);
	}

	if ((http_env[REQUEST_METHOD] != NULL) && (strcmp(http_env[REQUEST_METHOD], "POST") == 0))
	{
		http_env[CONTENT_LENGTH] = getenv("CONTENT_LENGTH");
		http_env[CONTENT_TYPE] = getenv("CONTENT_TYPE");
		
		if (http_env[CONTENT_LENGTH] != NULL)
		{
			wp.content_length = atoi(http_env[CONTENT_LENGTH]);
		}
		else
		{
			FCGI_ErrLog("get CONTENT_LENGTH error!\n");
		}
		
		if (http_env[CONTENT_TYPE] != NULL)
		{
			wp.content_type = http_env[CONTENT_TYPE];
			wp.content = calloc(1, wp.content_length + 1);
			if (wp.content == NULL){FCGI_ErrLog("calloc wp.content error! abort parse wp.content.\n");return &wp;}
	        if (fread(wp.content, wp.content_length, 2, stdin) != 1)
			{FCGI_ErrLog("%s CONTENT_LENGTH error! Data Read not Complete\n", http_env[CONTENT_TYPE]);}
			
			if (strstr(http_env[CONTENT_TYPE], "multipart/form-data") == NULL)
			{	
				ParseQuery_Add(wp.content);
			}
			else if (strstr(http_env[CONTENT_TYPE], "multipart/form-data") != NULL)
			{
				memset(&callbacks, 0, sizeof(multipart_parser_settings));
				callbacks.on_header_field = read_header_name;
				callbacks.on_header_value = read_header_value;
				callbacks.on_part_data = write_part;
				callbacks.on_part_data_begin = start_data_write;
				callbacks.on_part_data_end = end_data_write;
				callbacks.on_body_end = parse_end;
				/*--${bound}*/
				bound = strstr(http_env[CONTENT_TYPE], "boundary=") + 9;
				boundlen = strlen(bound);
				__bound = alloca(boundlen + 2);
				if(__bound == NULL){FCGI_ErrLog("alloca __bound error! abort parse multipart/form-data.\n");return &wp;}
				memcpy(__bound, "--", 2);memcpy(__bound + 2, bound, boundlen);
				__bound[boundlen + 2] = '\0';
				parser = multipart_parser_init(__bound, &callbacks);
				if (wp.content_length != (multipart_parser_execute_len = multipart_parser_execute(parser, wp.content, wp.content_length)))
				{
					FCGI_ErrLog("%s CONTENT_LENGTH:%d multipart_parser_execute_len:%d not equal! multipart_parser_execute not Complete!\n",
								http_env[CONTENT_TYPE], wp.content_length, multipart_parser_execute_len);
				}
				ParseQuery_Add(post_form_data);//将报文中非文件流的post数据解析出来
			}	
		}
		else
		{
			FCGI_ErrLog("get CONTENT_TYPE error!\n");
		}
	}
	wp.files = uploadfile_count;
	return &wp;
}

void FCGI_FreeWp(Webs * fwp)
{
	if (fwp != NULL)
	{
		if (fwp->content != NULL)
		{
			free(fwp->content);
			fwp->content = NULL;
		}
		if ((fwp->content_type != NULL) && (strstr(fwp->content_type, "multipart/form-data") != NULL))
		{
			multipart_parser_free(parser);
			parser = NULL;
			//需要删除所有上传的临时文件
			DeleteFile(NULL);
			//free post-form data
			if (post_form_data != NULL)
			{
				free(post_form_data);
				post_form_data = NULL;
				post_form_data_len = 0;
			}
		}
		if (fwp->query_string != NULL)
		{
			free(fwp->query_string);
			fwp->query_string = NULL;
		}
		memset(fwp, 0, sizeof(Webs));
	}
	else if (wp.method != NULL)
	{
		if (wp.content != NULL)
		{
			free(wp.content);
			wp.content = NULL;
		}
		if ((wp.content_type != NULL) && (strstr(wp.content_type, "multipart/form-data") != NULL))
		{
			multipart_parser_free(parser);
			parser = NULL;
			//需要删除所有上传的临时文件
			DeleteFile(NULL);
			//free post-form data
			if (post_form_data != NULL)
			{
				free(post_form_data);
				post_form_data = NULL;
				post_form_data_len = 0;
			}
		}
		if (wp.query_string != NULL)
		{
			free(wp.query_string);
			wp.query_string = NULL;
		}
		memset(&wp, 0, sizeof(Webs));
	}
}

int count = 0;
void _FCGI_PrintTest(char * TestMessage, char * func, int line)
{
	printf("Content-type: text/html\r\n"  
               "\r\n"  
               "<title>FastCGI Hello!</title>"  
               "<h1>FastCGI Hello!</h1>"  
               "Request number %d running on host <i>%s</i></p><i>%s @ %s:%d</i>\n",  
                ++count, getenv("SERVER_NAME"), TestMessage, func, line);  
}

MACRO_STR_T g_http_code_str[] ={  
	MACRO_STR(HTTP_CODE_CONTINUE),
	MACRO_STR(HTTP_CODE_OK),
	MACRO_STR(HTTP_CODE_CREATED),
	MACRO_STR(HTTP_CODE_ACCEPTED),
	MACRO_STR(HTTP_CODE_NOT_AUTHORITATIVE),
	MACRO_STR(HTTP_CODE_NO_CONTENT),
	MACRO_STR(HTTP_CODE_RESET),
	MACRO_STR(HTTP_CODE_PARTIAL),
	MACRO_STR(HTTP_CODE_MOVED_PERMANENTLY),
	MACRO_STR(HTTP_CODE_MOVED_TEMPORARILY),
	MACRO_STR(HTTP_CODE_SEE_OTHER),
	MACRO_STR(HTTP_CODE_NOT_MODIFIED),
	MACRO_STR(HTTP_CODE_USE_PROXY),
	MACRO_STR(HTTP_CODE_TEMPORARY_REDIRECT),
	MACRO_STR(HTTP_CODE_BAD_REQUEST),
	MACRO_STR(HTTP_CODE_UNAUTHORIZED),
	MACRO_STR(HTTP_CODE_PAYMENT_REQUIRED),
	MACRO_STR(HTTP_CODE_FORBIDDEN),
	MACRO_STR(HTTP_CODE_NOT_FOUND),
	MACRO_STR(HTTP_CODE_BAD_METHOD),
	MACRO_STR(HTTP_CODE_NOT_ACCEPTABLE),
	MACRO_STR(HTTP_CODE_REQUEST_TIMEOUT),
	MACRO_STR(HTTP_CODE_CONFLICT),
	MACRO_STR(HTTP_CODE_GONE),
	MACRO_STR(HTTP_CODE_LENGTH_REQUIRED),
	MACRO_STR(HTTP_CODE_PRECOND_FAILED),
	MACRO_STR(HTTP_CODE_REQUEST_TOO_LARGE),
	MACRO_STR(HTTP_CODE_REQUEST_URL_TOO_LARGE),
	MACRO_STR(HTTP_CODE_UNSUPPORTED_MEDIA_TYPE),
	MACRO_STR(HTTP_CODE_RANGE_NOT_SATISFIABLE),
	MACRO_STR(HTTP_CODE_EXPECTATION_FAILED),
	MACRO_STR(HTTP_CODE_NO_RESPONSE),
	MACRO_STR(HTTP_CODE_INTERNAL_SERVER_ERROR),
	MACRO_STR(HTTP_CODE_NOT_IMPLEMENTED),
	MACRO_STR(HTTP_CODE_BAD_GATEWAY),
	MACRO_STR(HTTP_CODE_SERVICE_UNAVAILABLE),
	MACRO_STR(HTTP_CODE_GATEWAY_TIMEOUT),
	MACRO_STR(HTTP_CODE_BAD_VERSION),
	MACRO_STR(HTTP_CODE_INSUFFICIENT_STORAGE),	   
	MACRO_STR(HTTP_CODE_START_LOCAL_ERRORS),
	MACRO_STR(HTTP_CODE_COMMS_ERROR),
	MACRO_STR(HTTP_CODE_END),
	{-1, NULL}  
};

char * lib_fcgitools_get_macro_name(MACRO_STR_T* table, int id)  
{  
   int i = 0;  
 
   while(table[i].id != -1 && table[i].name != NULL){  
	   if(table[i].id == id)  
		   return &table[i].name[10];  
 
	   i++;  
   }  
   return "";  
}  

void _FCGI_PrintWeb(int Status,char * Content_type,char * RedirectUrlorMessage, char * func, int line)
{
	switch(Status)
	{
		case HTTP_CODE_MOVED_TEMPORARILY:
			printf("Status: 302 Moved Temporarily\r\n"
		            "Location: %s\r\n"
		            "URI: %s\r\n"
		            "Connection: close\r\n"
		            "Content-type: %s; charset=UTF-8\r\n\r\n",RedirectUrlorMessage,RedirectUrlorMessage,Content_type);//text/html
			break;
		case HTTP_CODE_OK:
			printf("Content-type: %s\r\n\r\n", Content_type);
			printf("%s",RedirectUrlorMessage);
			break;
		case HTTP_CODE_NO_CONTENT:
			printf("Status: 204 Not Content\r\n");
			printf("Content-type: %s\r\n\r\n", Content_type);
			printf("%s",RedirectUrlorMessage);
			break;
		default:
			printf("Status: %d %s\r\n"
		            "Connection: close\r\n"
		            "Content-type: text/html; charset=UTF-8\r\n\r\n", Status, lib_fcgitools_get_macro_name(g_http_code_str, Status));
			printf("<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
				    "<html><head>\n"
				    "<title>Set Status %d Internal Server Error</title>\n"
				    "<style type=\"text/css\">\n"
				    "h1 {color:#5E94A2} \n"
				    "p {color:#385961} \n"
				    "b {color:#F93A86} \n"
					"body {background-image:url(/error.jpg);background-repeat: no-repeat;background-position:50%% 1500%%;}\n"
					"</style></head><body>\n"
				    "<h1>Internal Server Error </p>Content_type:<b>%s</b> RedirectUrlorMessage:<b>%s</b></h1>\n"
				    "<p>The server encountered an internal error in <b>%s:%d</b>\n"
				    "or misconfiguration and was unable to complete\n"
				    "your request.</p>\n"
				    "<p>Please contact the server administrator CreatCommNJ\n"
				    "<p>More information about this error may be available\n"
				    "in the server error log.</p>\n"
				    "</body></html>\n", Status, Content_type, RedirectUrlorMessage, func, line);
			break;
	}
}

void FCGI_SetSession(char * Key,char * Value)
{
	if(Key != NULL && strlen(Key) != 0)
	{
		printf("Content-Type: text/plain\r\n");
		printf("X-Replace-Session: %s=%s\r\n", Key, urlencode((Value != NULL) ? Value : ""));
	}
	else if(strlen(X_Replace_Session) != 0)
    {
		printf("Content-Type: text/plain\r\n");
    	printf("X-Replace-Session: %s\r\n", X_Replace_Session);
    }
}

inline void ZeroMsg(void)
{
	memset(szMsg, 0, sizeof(szMsg));
	if (pMsg != szMsg && pMsg != NULL)
	{
		memset(pMsg, 0, MSG_LEN);
	}
}

inline int ExternMsgMem(void)
{
	if (msg_block >= MAX_MSG_BLOCK) 
	{
		FCGI_ErrLog("msg_block is touch limit, cur block is %d, max block is %d, msg len not externed, cur len is %d\n",
					msg_block, MAX_MSG_BLOCK, MSG_LEN);
		return 2;
	}
	msg_block++;
	char * p_realloc = NULL;
	if (msg_block > 2) {p_realloc = pMsg;}
	pMsg = (char *)realloc(p_realloc, msg_block * MAX_APACHE_MSG_LEN);
	if (pMsg == NULL)
	{
		pMsg = p_realloc;
		msg_block--;
		FCGI_ErrLog("realloc error, msg len not externed, cur len is %d\n", MSG_LEN);
		return 1;
	}
	if (pMsg != NULL && msg_block == 2)
	{
		memcpy(pMsg, szMsg, MAX_APACHE_MSG_LEN);		
	}
	return 0;
}

inline int FCGI_websWrite(char* Message, char* format, ...)
{
	char tempmsg[MAX_APACHE_TEMPMSG_LEN] = {0};
	int oldlen = 0,addlen = 0;
	int ret = 0;
	int msg_len = MSG_LEN;
	oldlen = strlen(Message);
	va_list vArgList;
	va_start(vArgList, format);
	ret = vsnprintf(tempmsg, MAX_APACHE_TEMPMSG_LEN, format, vArgList);
	va_end(vArgList);
	addlen = strlen(tempmsg);
	if (oldlen + addlen >= msg_len && ExternMsgMem() != 0)
	{	
		FCGI_ErrLog("%s is too long for fcgi msg!max:%d has:%d cur:%d\n",
					tempmsg, msg_len, oldlen, addlen);
		addlen = msg_len - oldlen - 1;
		ret = -1;
	}
	if (Message != pMsg) {Message = pMsg;}
	memcpy(&Message[oldlen], tempmsg, addlen);
	Message[oldlen + addlen + 1] = '\0';
	return ret;

}

char encode_result[MAX_ENCODE_LEN] = {0};
char * urlencode(char const *s)
{
    char const *from, *end;
	char c;
	char * to = NULL;
    from = s;
    end = s + strlen(s);
    to = encode_result;
	memset(encode_result, 0, sizeof(encode_result));
    unsigned char hexchars[] = "0123456789ABCDEF";
    while (from < end) {
        c = *from++;

        if (c == ' ') {
            *to++ = '+';
        } else if ((c < '0' && c != '-' && c != '.')
                   ||(c < 'A' && c > '9')
                   ||(c > 'Z' && c < 'a' && c != '_')
                   ||(c > 'z')) {
            to[0] = '%';
            to[1] = hexchars[c >> 4];
            to[2] = hexchars[c & 15];
            to += 3;
        } else {
            *to++ = c;
        }
    }
    *to = '\0';
    return (char *) encode_result;
}

char * urldecode(char *p)
{
	char * url = p;
	unsigned int i = 0;  
	while(*(p+i))  
	{  
		if((*p=*(p+i)) == '%')  
		{  
			*p=*(p+i+1) >= 'A' ? ((*(p+i+1) & 0XDF) - 'A') + 10 : (*(p+i+1) - '0');  
			*p=(*p) * 16;  
			*p+=*(p+i+2) >= 'A' ? ((*(p+i+2) & 0XDF) - 'A') + 10 : (*(p+i+2) - '0');  
			i+=2;  
		}  
		else if(*(p+i)=='+')  
		{  
			*p=' ';  
		}  
		p++;  
	}  
	*p='\0';
	return url;
}  

char base64_result[MAXL_BASE64CODE] = {0};
char * base64_encode(const char* str)
{  
    int     n=0;  
    FILE*   fp=NULL;    
    char    buf[256];  
	if (str == NULL || strlen(str) == 0)
	{
		return "";
	}

	memset(base64_result, 0, MAXL_BASE64CODE);
    sprintf(buf, "echo -n \"%s\" | base64 -w 0", str);  
      
    if(NULL == (fp=popen(buf,"r")))    
    {   
        fprintf(stderr, "execute command failed: %s", strerror(errno));    
        return "\0";    
    }   
    if(NULL == fgets(base64_result, MAXL_BASE64CODE, fp))   
    {  
        pclose(fp);  
        return "\0";  
    }  
      
    n = strlen(base64_result);  
    if('\n' == base64_result[n-1])  
        base64_result[n-1] = 0;   // 去掉base64命令输出的换行符  
          
    pclose(fp);  
      
    return base64_result;   
}  
  
char * base64_decode(const char* str)
{  
    int     n=0;  
    FILE*   fp=NULL;    
    char    buf[256];  
	if (str == NULL || strlen(str) == 0)
	{
		return "";
	}

	memset(base64_result, 0, MAXL_BASE64CODE);
    sprintf(buf, "echo %s | base64 -d", str);  
      
    if(NULL == (fp=popen(buf, "r")))    
    {   
        fprintf(stderr, "execute command failed: %s", strerror(errno));    
        return "\0";    
    }   
    if(NULL == fgets(base64_result, MAXL_BASE64CODE, fp))   
    {  
        pclose(fp);  
        return "\0";  
    }  
      
    n = strlen(base64_result);  
    if('\n' == base64_result[n-1])  
        base64_result[n-1] = 0;   // 去掉base64命令输出的换行符  
          
    pclose(fp);  
      
    return base64_result;   
}  


int findstr(char* src, char* s)
{
    char *ptr=src, *p=s;    //定义两个指针
    char *ptr2=src+strlen(src), *prev=NULL;    //ptr2为src的末位置指针
    int len=strlen(s), n=0;        //子串的长度和计数器
    for(;*ptr;ptr++)    //循环整个串
    {
        if(ptr2-ptr<len)    //如果一开始子串就大于src,则退出
            break;
        for(prev=ptr;*prev==*p;prev++,p++)    //寻找第一个相等的位置,然后从此位置开始匹配子串
        {
            if(*(p+1)==0||*(p+1)==10)    //如果已经到了子串的末尾
            {
                n++;    //自增
                p=s;    //重新指向子串
                break;//退出
            }
        }
    }
    return n;
}

char * FCGI_GetPostFormData_clientFileName(int fileindex)
{
	if (fileindex >= MAX_UPLOADFILE_COUNT) {return &Null_String;}
	return uploadfile_name[fileindex];
}

char * FCGI_GetPostFormData_FileName(int fileindex)
{
	if (fileindex >= MAX_UPLOADFILE_COUNT) {return &Null_String;}
	int prefix_len = strlen(UPLOAD_FILE_PREFIX);
	int file_name_len = strlen(uploadfile_name[fileindex]);
	memcpy(uploadfile_full_name[fileindex], UPLOAD_FILE_PREFIX, prefix_len);
	memcpy(uploadfile_full_name[fileindex] + prefix_len, uploadfile_name[fileindex], file_name_len);
	uploadfile_full_name[fileindex][prefix_len + file_name_len] = '\0';
	return uploadfile_full_name[fileindex];
}

unsigned int FCGI_GetPostFormData_FileLength(int fileindex)
{
	if (fileindex >= MAX_UPLOADFILE_COUNT) {return 0;}
	return uploadfile_size[fileindex];
}

char * FCGI_GetPostFormData_FileContentType(int fileindex)
{
	if (fileindex >= MAX_UPLOADFILE_COUNT) {return &Null_String;}
	return uploadfile_content_type[fileindex];
}

