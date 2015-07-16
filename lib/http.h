/**
 * @file   http.h
 * @author mathslinux <riegamaths@gmail.com>
 * @date   Mon May 21 23:07:29 2012
 *
 * @brief  Linux WebQQ Http API
 *
 *
 */

#ifndef LWQQ_HTTP_H
#define LWQQ_HTTP_H

#include "type.h"

typedef struct LwqqAsyncEvent LwqqAsyncEvent;
typedef struct LwqqExtension LwqqExtension;

typedef enum {
   LWQQ_FORM_FILE, // use add_file_content instead
   LWQQ_FORM_CONTENT
} LWQQ_FORM;
typedef enum {
   LWQQ_HTTP_TIMEOUT, // connection timeout
   LWQQ_HTTP_TIMEOUT_INCRE, // auto increment timeout
   LWQQ_HTTP_NOT_FOLLOW,
   LWQQ_HTTP_SAVE_FILE,
   LWQQ_HTTP_RESET_URL,
   LWQQ_HTTP_VERBOSE,
   LWQQ_HTTP_CANCELABLE,
   LWQQ_HTTP_MAXREDIRS,
   LWQQ_HTTP_MAX_LINK = 1000
} LwqqHttpOption;
/**
 * Lwqq Http request struct, this http object worked done for lwqq,
 * But for other app, it may work bad.
 *
 */
typedef struct LwqqHttpRequest {
   void* req;
   void* header; // read and write.
   void* recv_head;
   void* form_start;
   void* form_end;

   /**
    * Http code return from server. e.g. 200, 404, this maybe changed
    * after do_request() called.
    */
   short http_code;
   short retry;

   /* Server response, used when do async request */
   // char *location;
   char* response;

   /* Response length, NB: the response may not terminate with '\0' */
   size_t resp_len;

   /**
    * Send a request to server, method is GET(0) or POST(1), if we make a
    * POST request, we must provide a http body.
    */
   int (*do_request)(LwqqHttpRequest* request, int method, char* body);

   /**
    * Send a request to server asynchronous, method is GET(0) or POST(1),
    * if we make a POST request, we must provide a http body.
    */
   LwqqAsyncEvent* (*do_request_async)(LwqqHttpRequest* request, int method,
                                       char* body, LwqqCommand);

   /* Set our http client header */
   void (*set_header)(LwqqHttpRequest* request, const char* name,
                      const char* value);

   /** Get header */
   const char* (*get_header)(LwqqHttpRequest* request, const char* name);

   // add http form
   void (*add_form)(LwqqHttpRequest* request, LWQQ_FORM form, const char* name,
                    const char* content);
   // add http form file type
   void (*add_file_content)(LwqqHttpRequest* request, const char* name,
                            const char* filename, const void* data, size_t size,
                            const char* extension);
   // progressing function callback
   LwqqProgressFunc progress_func;
   void* prog_data;
   time_t last_prog;
} LwqqHttpRequest;

typedef struct LwqqHttpHandle {
   struct {
      enum {
         LWQQ_HTTP_PROXY_NOT_SET = -1, // let curl auto set proxy
         LWQQ_HTTP_PROXY_NONE,
         LWQQ_HTTP_PROXY_HTTP,
         LWQQ_HTTP_PROXY_SOCKS4,
         LWQQ_HTTP_PROXY_SOCKS5
      } type;
      char* host;
      int port;
      char* username;
      char* password;
   } proxy;
   int quit;
   int synced;
   int ssl;
} LwqqHttpHandle;

LwqqHttpHandle* lwqq_http_handle_new();
void lwqq_http_handle_free(LwqqHttpHandle* http);
#define lwqq_http_proxy_set(_handle, _type, _host, _port, _username,           \
                            _password)                                         \
   do {                                                                        \
      LwqqHttpHandle* h = (LwqqHttpHandle*)(_handle);                          \
      h->proxy.type = _type;                                                   \
      h->proxy.host = s_strdup(_host);                                         \
      h->proxy.port = _port;                                                   \
      h->proxy.username = s_strdup(_username);                                 \
      h->proxy.password = s_strdup(_password);                                 \
   } while (0);

void lwqq_http_proxy_apply(LwqqHttpHandle* handle, LwqqHttpRequest* req);

/**
 * Free Http Request
 * always return 0
 *
 * @param request
 */
int lwqq_http_request_free(LwqqHttpRequest* request);

/**
 * Create a new Http request instance
 *
 * @param uri Request service from
 *
 * @return
 */
LwqqHttpRequest* lwqq_http_request_new(const char* uri);

/**
 * Create a default http request object using default http header.
 *
 * @param url Which your want send this request to
 * @param err This parameter can be null, if so, we dont give thing
 *        error information.
 *
 * @return Null if failed, else a new http request object
 */
LwqqHttpRequest* lwqq_http_create_default_request(LwqqClient* lc,
                                                  const char* url,
                                                  LwqqErrorCode* err);

// get a fake event from request, only used to get errno. shouldn't use
// lwqq_async function on it
LwqqAsyncEvent* lwqq_http_get_as_ev(LwqqHttpRequest*);
#define LWQQ_HTTP_EV(req) lwqq_http_get_as_ev(req)
/** return a string based on impl errno */
const char* lwqq_http_impl_errstr(int errno);

void lwqq_http_global_init();
void lwqq_http_global_free(LwqqCleanUp cleanup);
/** stop a client all http progressing request */
void lwqq_http_cleanup(LwqqClient* lc, LwqqCleanUp cleanup);
/** set the other option of request, like curl_easy_setopt */
void lwqq_http_set_option(LwqqHttpRequest* req, LwqqHttpOption opt, ...);
/** regist http progressing callback */
void lwqq_http_on_progress(LwqqHttpRequest* req, LwqqProgressFunc progress,
                           void* prog_data);
const char* lwqq_http_get_url(LwqqHttpRequest* req);
int lwqq_http_is_synced(LwqqHttpRequest* req);
/** get cookie which key==name,
 *  req's url affect cookie list,
 *  if a cookie registerd with domain 'web2.qq.com'
 *  then a req with url='pp.com' couldn't get this cookie
 */
char* lwqq_http_get_cookie(LwqqHttpRequest* req, const char* name);
/** add a cookie with name=val to request,
 * if @store, this would store in cache,
 * if not, this would only affect single request
 */
void lwqq_http_set_cookie(LwqqHttpRequest* request, const char* name,
                          const char* val, int store);
/**
 * force stop a request
 * require set LWQQ_HTTP_CANCELABLE option first
 * invoke callback with failcode = LWQQ_EC_CANCELED
 */
void lwqq_http_cancel(LwqqHttpRequest* req);
/**
 * a standard on http when verbose level >= lv
 * then print out verbose information
 */
#define lwqq_http_debug(req, lv)                                               \
   LWQQ_DEBUG(if (LWQQ_VERBOSE_LV >= lv)                                       \
              lwqq_http_set_option(req, LWQQ_HTTP_VERBOSE, 1L);)

LwqqExtension* lwqq_make_cookie_extension(LwqqClient* lc, const char* filename);

#endif /* LWQQ_HTTP_H */

