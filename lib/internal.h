#ifndef LWQQ_INTERNAL_H_H
#define LWQQ_INTERNAL_H_H
#include "lwqq-config.h"

#ifdef WIN32
#include "lwqq_export.h"
#include "win32.h"
#else
#define LWQQ_EXPORT
#endif

#define WEBQQ_VERSION "20130916001"
#define WEBQQ_APPID "1003903"
#define WQQ_APPID "501004106"
#define JS_VER "10120"

#define WEBQQ_LOGIN_UI_HOST "https://ui.ptlogin2.qq.com"
#define WEBQQ_CAPTCHA_HOST "https://ssl.captcha.qq.com"
#define WEBQQ_LOGIN_HOST "https://ssl.ptlogin2.qq.com"
#define WEBQQ_CHECK_HOST WEBQQ_LOGIN_HOST
#define WEBQQ_S_HOST "http://s.web2.qq.com"
#define WEBQQ_HOST "http://web2.qq.com"
#define WQQ_HOST "http://w.qq.com"

#define WEBQQ_S_REF_URL                                                        \
   WEBQQ_S_HOST "/proxy.html?v=" WEBQQ_VERSION "&callback=1&id=1"
#define WEBQQ_LOGIN_REF_URL WEBQQ_LOGIN_HOST "/proxy.html"
#define WEBQQ_VERSION_URL WEBQQ_LOGIN_UI_HOST "/cgi-bin/ver"
#define WEBQQ_VERSION_URL WEBQQ_LOGIN_UI_HOST "/cgi-bin/ver"
#define WEBQQ_LOGIN_LONG_REF_URL(buf)                                          \
   (snprintf(buf, sizeof(buf), WEBQQ_LOGIN_UI_HOST                             \
             "/cgi-bin/login?daid=164&target=self&style=5&mibao_css=m_webqq"   \
             "&appid=1003903&enable_qlogin=0&no_verifyimg=1&s_url=http%%3A%%"  \
             "2F%%2Fweb2.qq.com"                                               \
             "%%2Floginproxy.html&f_url=loginerroralert&strong_login=1&login_" \
             "stat=%d&t=%lu",                                                  \
             lc->stat, LTIME),                                                 \
    buf)
#define WQQ_LOGIN_LONG_REF_URL(buf)                                            \
   (snprintf(buf, sizeof(buf), WEBQQ_LOGIN_UI_HOST                             \
             "/cgi-bin/login?daid=164&target=self&style=5&mibao_css=m_webqq"   \
             "&appid=" WQQ_APPID                                               \
             "&enable_qlogin=0&no_verifyimg=1&s_url=http%%3A%%"                \
             "2F%%2Fw.qq.com"                                                  \
             "%%2Fproxy.html&f_url=loginerroralert&strong_login=1&login_"      \
             "stat=%d&t=%lu",                                                  \
             lc->stat, LTIME),                                                 \
    buf)

#define lwqq_http_ssl(lc) (lwqq_get_http_handle(lc)->ssl)
#define __SSL lwqq_http_ssl(lc)
#define __H(url) __SSL ? "https://" url : "http://" url
#define WEBQQ_D_REF_URL                                                        \
   (__SSL) ? "https://d.web2.qq.com/cfproxy.html?v=" WEBQQ_VERSION             \
             "&callback=1"                                                     \
           : "http://d.web2.qq.com/proxy.html?v=" WEBQQ_VERSION "&callback=1"
#define WEBQQ_D_HOST __H("d.web2.qq.com")
// at now, only get_offpic2 use WQQ_D_HOST
#define WQQ_D_HOST "http://w.qq.com/d"

typedef struct json_value json_t;
typedef struct LwqqAsyncEvent LwqqAsyncEvent;
typedef struct LwqqClient LwqqClient;
typedef struct LwqqVerifyCode LwqqVerifyCode;
typedef struct LwqqHttpRequest LwqqHttpRequest;

struct str_list_ {
   char* str;
   struct str_list_* next;
};
struct str_list_* str_list_prepend(struct str_list_* list, const char* str);
#define str_list_free_all(list)                                                \
   while (list != NULL) {                                                      \
      struct str_list_* ptr = list;                                            \
      list = list->next;                                                       \
      s_free(ptr->str);                                                        \
      s_free(ptr);                                                             \
   }

int lwqq__get_retcode_from_str(const char* str);
json_t* lwqq__parse_retcode_result(json_t* json, int* retcode);

LwqqAsyncEvent* lwqq__request_captcha(LwqqClient* lc, LwqqVerifyCode* c);
int lwqq__process_empty(LwqqHttpRequest* req);
int lwqq__process_simple_response(LwqqHttpRequest* req);

// check http response is 200 OK
#define lwqq__jump_if_http_fail(req, err)                                      \
   if (req->http_code != 200) {                                                \
      err = LWQQ_EC_ERROR;                                                     \
      goto done;                                                               \
   }
// parse http response as json object
#define lwqq__jump_if_json_fail(json, str, err)                                \
   if (str == NULL || json_parse_document(&json, str) != JSON_OK) {            \
      lwqq_log(LOG_ERROR, "Parse json object from response failed: %s\n",      \
               str);                                                           \
      err = LWQQ_EC_NOT_JSON_FORMAT;                                           \
      goto done;                                                               \
   }

#define lwqq__jump_if_retcode_fail(retcode)                                    \
   if (retcode != LWQQ_EC_OK)                                                  \
      goto done;

#define lwqq__jump_if_ev_fail(ev, err)                                         \
   do {                                                                        \
      if (ev == NULL || ev->result != LWQQ_EC_OK) {                            \
         err = LWQQ_EC_ERROR;                                                  \
         goto done;                                                            \
      }                                                                        \
   } while (0);

#define lwqq__return_if_ev_fail(ev)                                            \
   do {                                                                        \
      if (ev == NULL || ev->result != LWQQ_EC_OK)                              \
         return;                                                               \
   } while (0);

#define lwqq__clean_json_and_req(json, req)                                    \
   do {                                                                        \
      if (json)                                                                \
         json_free_value(&json);                                               \
      lwqq_http_request_free(req);                                             \
   } while (0);

#define lwqq__log_if_error(err, req)                                           \
   if (err)                                                                    \
      lwqq_log(LOG_ERROR,                                                      \
               "unexpected response \n\thttp:%d, response:\n\t%s\n",           \
               req->http_code, req->response);
#define lwqq__has_post() (lwqq_verbose(3, "%s\n%s\n", url, post), 1), post
#define lwqq__hasnot_post() (lwqq_verbose(3, "%s\n", url), 0), NULL

/** ===================json part==================*/
#define lwqq__json_get_int(json, k, def)                                       \
   s_atoi(json_parse_simple_value(json, k), def)

#define lwqq__json_get_long(json, k, def)                                      \
   s_atol(json_parse_simple_value(json, k), def)

#define lwqq__json_get_value(json, k) s_strdup(json_parse_simple_value(json, k))

#define lwqq__json_get_string(json, k)                                         \
   json_unescape_s(json_parse_simple_value(json, k))

#define lwqq__json_parse_child(json, k, sub)                                   \
   sub = json_find_first_label(json, k);                                       \
   if (sub)                                                                    \
      sub = sub->child;

// json function expand
json_t* json_find_first_label_all(const json_t* json, const char* text_label);
char* json_parse_simple_value(json_t* json, const char* key);
char* json_unescape_s(char* str);

#endif

