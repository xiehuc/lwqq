/**
 * @file   login.c
 * @author mathslinux <riegamaths@gmail.com>
 * @date   Sun May 20 02:21:43 2012
 *
 * @brief  Linux WebQQ Login API
 *  Login logic is based on the gtkqq implementation
 *  written by HuangCongyu <huangcongyu2006@gmail.com>
 *
 *
 */

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include "login.h"
#include "logger.h"
#include "http.h"
#include "smemory.h"
#include "url.h"
#include "json.h"
#include "async.h"
#include "utility.h"
#include "internal.h"
#include "lwjs.h"
#include "info.h"

#define replace(str, f, t)                                                     \
   do {                                                                        \
      char* chr = str;                                                         \
      while ((chr = strchr(chr, f)))                                           \
         *chr = t;                                                             \
   } while (0)

struct LoginStage{
  unsigned stage;
  LwqqErrorCode err;
  LwqqAsyncEvent* trigger;
  LwqqAsyncEvent* ev;
  char* vcode;
  char* salt;
  char* check_sig_url;
  char randSalt;
};

typedef LwqqAsyncEvent* (*LoginFunc)(LwqqClient*, struct LoginStage*);

static void do_login_stage(LwqqClient* lc, struct LoginStage* s);

/** stage 1 **/
static LwqqAsyncEvent* get_login_sig(LwqqClient* lc, struct LoginStage* s)
{
   char url[512];
   snprintf(url, sizeof(url), WEBQQ_LOGIN_UI_HOST
            "/cgi-bin/login"
            "?daid=164&target=self&style=5&mibao_css=m_webqq&appid=" WQQ_APPID
            "&enable_qlogin=0&no_verifyimg=1&s_url=http%%3A%%2F%%2Fw.qq.com%%"
            "2Fproxy.html?f_url=loginerroralert&strong_login=1&login_state=10");
   LwqqHttpRequest* req = lwqq_http_create_default_request(lc, url, NULL);
   lwqq_http_set_option(req, LWQQ_HTTP_TIMEOUT, 5);
   lwqq_http_set_option(req, LWQQ_HTTP_TIMEOUT_INCRE, 5);

   ++s->stage; // this is always success

   return req->do_request_async(req, lwqq__hasnot_post(),
                                _C_(p, lwqq_http_request_free, req));
}
static int check_need_verify_cb(LwqqHttpRequest* req, struct LoginStage* s);
/** stage 2 **/
static LwqqAsyncEvent* check_need_verify(LwqqClient* lc, struct LoginStage* s)
{
   LwqqHttpRequest* req;
   char url[512];
   char buf[256];

   srand48(time(NULL));
   double random = drand48();
   snprintf(url, sizeof(url), WEBQQ_CHECK_HOST
            "/check?pt_tea=1&uin=%s&appid=" WQQ_APPID "&"
            "js_ver=" JS_VER "&js_type=0&login_sig=%s&pt_tea=1&u1=http%%3A%%2F%"
                             "%2Fw.qq.com%%2Fproxy.html&r=%.16lf",
            lc->username, "", random);
   req = lwqq_http_create_default_request(lc, url, NULL);
   req->set_header(req, "Referer", WQQ_LOGIN_LONG_REF_URL(buf));
   lwqq_http_debug(req, 5);

   return req->do_request_async(req, lwqq__hasnot_post(),
                                _C_(2p_i, check_need_verify_cb, req, s));
}

static int check_need_verify_cb(LwqqHttpRequest* req, struct LoginStage* s)
{
   int err = LWQQ_EC_OK;
   LwqqClient* lc = req->lc;
   if (req->err != LWQQ_EC_OK) {
      err = LWQQ_EC_NETWORK_ERROR;
      goto done;
   }
   lwqq__jump_if_http_fail(req, err);
   /**
    *
    ptui_checkVC('0','!ZDP','\x96\x00\xb9\xd6\xcf\x87\xae\xf4','34bfca26ba2613a99cfc2d62116d8217cda391c14a07f38523eeb7be2542f1d14b3ab7f93c0deeb782dfc5e723bc055952acf969ebf7d1c4','1');

    ptui_checkVC('1','bk9gaJlbRSmYU-nOXKxvPaAxjzPHr99tjldA-NhvvyRe2YxlAt94yA**','\x00\x00\x00\x00\x95\x1a\x82\x5c','','0');
    */

   int need_vf;
   char vc[256] = { 0 };
   char salt[256] = { 0 };
   char vf[256] = { 0 };
   s->randSalt = '0';
   sscanf(req->response,
          "ptui_checkVC('%d','%255[^']','%255[^']','%255[^']','%c');", &need_vf,
          vc, salt, vf, &s->randSalt);
   s->vcode = s_strdup(vc);
   s->salt = s_strdup(salt);
   /*lc->vc = s_malloc0(sizeof(*lc->vc));
   lc->vc->uin = s_strdup(salt);
   lc->vc->str = s_strdup(vc);
   lc->vc->lc = lc;*/

   if (need_vf == 0) {
      s->stage += 2; // ignore get_verify_image
      /* We need get the ptvfsession from the header "Set-Cookie" */
      lwqq_log(LOG_NOTICE, "Verify code: %s\n", vc);
   } else if (need_vf == 1) {
      s->stage ++;
      lwqq_log(LOG_NOTICE, "We need verify code image: %s\n", vc);
   } else {
      assert(0 && "unexpected result");
   }

   lwqq_override(lc->session.pt_verifysession, s_strdup(vf));
done:
   lwqq_http_request_free(req);
   return err;
}

static int get_verify_image_cb(LwqqHttpRequest* req, struct LoginStage* s);
/** stage 3 **/
static LwqqAsyncEvent* get_verify_image(LwqqClient* lc, struct LoginStage* s)
{
   LwqqHttpRequest* req = NULL;
   char url[512];
   char buf[256];
   LwqqErrorCode err;
   srand48(time(NULL));
   double random = drand48();

   snprintf(url, sizeof(url),
            WEBQQ_CAPTCHA_HOST "/getimage?aid=" WQQ_APPID "&uin=%s&%.16f&cap_cd=%s",
            lc->username, random, s->vcode);
   req = lwqq_http_create_default_request(lc, url, &err);
   req->set_header(req, "Referer", WQQ_LOGIN_LONG_REF_URL(buf));
   lwqq_http_debug(req, 5);

   return req->do_request_async(req, lwqq__hasnot_post(),
                                _C_(2p_i, get_verify_image_cb, req, s));
}
static int get_verify_image_cb(LwqqHttpRequest* req, struct LoginStage* s)
{
   int err = 0;
   lwqq__jump_if_http_fail(req, err);
   LwqqClient* lc = req->lc;

   lwqq_override(lc->session.pt_verifysession,
                 lwqq_http_get_cookie(req, "verifysession"));
   
   LwqqVerifyCode* code = s_malloc0(sizeof(*code));
   code->data = req->response;
   code->size = req->resp_len;
   req->response = NULL;
   code->cmd = _C_(2p, do_login_stage, lc, s);
   code->lc = lc;

   s->stage ++;
   lc->args->vf_image = code;
   vp_do_repeat(lc->events->need_verify, NULL);
   err = LWQQ_EC_LOGIN_NEED_VC;
done:
   lwqq_http_request_free(req);
   return err;
}

static int do_login_cb(LwqqHttpRequest* req, struct LoginStage* s);
/** stage 4 **/
static LwqqAsyncEvent* do_login(LwqqClient* lc, struct LoginStage* s)
{
   // caculate password
   char* js_txt = lwqq_util_load_res("encrypt.js", 1);
   lwqq_js_t* js = lwqq_js_init();
   lwqq_js_load_buffer(js, js_txt);
   replace(s->salt, '\\', '-');
   char* enc = lwqq_js_enc_pwd(lc->password, s->salt, s->vcode, js);
   s_free(js_txt);
   lwqq_js_close(js);

   char url[1024];
   char refer[1024];
   LwqqHttpRequest* req;

   snprintf(url, sizeof(url),
            WEBQQ_LOGIN_HOST "/login?"
                             "u=%s"
                             "&p=%s"
                             "&verifycode=%s"
                             "&webqq_type=%d"
                             "&remember_uin=1"
                             "&aid=" WQQ_APPID "&login2qq=1"
                             "&u1=http%%3A%%2F%%2Fw.qq.com%%2Fproxy.html%%"
                             "3Flogin2qq%%3D1%%26webqq_type%%3D10"
                             "&h=1"
                             "&ptredirect=0"
                             "&ptlang=2052"
                             "&daid=164"
                             "&from_ui=1"
                             "&pttype=1"
                             "&dumy="
                             "&fp=loginerroralert"
                             //"&action=0-0-5111"
                             "&mibao_css=m_webqq"
                             "&t=1"
                             "&g=1"
                             "&js_type=0"
                             "&js_ver=" JS_VER "&login_sig="
                             "&pt_uistyle=5"
                             "&pt_randsalt=%c"
                             "&pt_vcode_v1=0"
                             "&pt_verifysession_v1=%s",
            lc->username, enc,
            lc->args->vf_image ? lc->args->vf_image->str : s->vcode, lc->stat,
            s->randSalt, lc->session.pt_verifysession ?: "");
   s_free(enc);
   lwqq_vc_free(lc->args->vf_image);
   lc->args->vf_image = NULL;

   req = lwqq_http_create_default_request(lc, url, NULL);
   /* Setup http header */
   req->set_header(req, "Referer", WQQ_LOGIN_LONG_REF_URL(refer));
   lwqq_http_debug(req, 5);

   return req->do_request_async(req, lwqq__hasnot_post(),
                                _C_(2p_i, do_login_cb, req, s));
}

static int do_login_cb(LwqqHttpRequest* req, struct LoginStage* s)
{
   LwqqClient* lc = req->lc;
   int err = LWQQ_EC_OK;
   const char* response;
   char error_desc[512];
   // const char redirect_url[512];
   if (req->http_code != 200) {
      err = LWQQ_EC_HTTP_ERROR;
      goto done;
   }
   if (strstr(req->response, "aq.qq.com") != NULL) {
      err = LWQQ_EC_LOGIN_ABNORMAL;
      const char* beg = strstr(req->response, "http://aq.qq.com");
      if (beg) {
         sscanf(beg, "%511[^']", error_desc); // beg may be null
         lwqq_override(lc->error_description, s_strdup(error_desc));
      }
      goto done;
   }
   if (req->response == NULL) {
      lwqq_puts("login no response\n");
      err = LWQQ_EC_NETWORK_ERROR;
      goto done;
   }

   response = req->response;
   lwqq_verbose(3, "%s\n", response);
   //ptuiCB('71','0','','0','您的帐号由于异常禁止登录，请联系客服。(2009081021)', 'QQNUMBER');
   char* p = strstr(response, "\'");
   if (!p) {
      err = LWQQ_EC_ERROR;
      goto done;
   }
   int status, param2;
   char url[512];
   int param4;
   char msg[512];
   char user[64];
   // void url is '' which makes sscanf failed
   //%*c is for eat a blank before 'user'
   sscanf(response, "ptuiCB('%d','%d','%511[^,],'%d','%511[^']',%*c'%63[^']');",
          &status, &param2, url, &param4, msg, user);
   url[strlen(url) - 1] = 0;
   switch (status) {
   case 0:
      err = LWQQ_EC_OK;
      // going to check_sig
      s->check_sig_url = s_strdup(url);
      s->stage++;
      break;
   case 1:
      lwqq_log(LOG_WARNING, "Server busy! Please try again\n");
      err = LWQQ_EC_ERROR;
      break;

   case 2:
      lwqq_log(LOG_ERROR, "Out of date QQ number\n");
      err = LWQQ_EC_ERROR;
      break;

   case 3:
      lwqq_log(LOG_ERROR, "Wrong password\n");
      err = LWQQ_EC_WRONG_PASS;
      break;

   case 4:
      lwqq_log(LOG_ERROR, "Wrong verify code\n");
      err = LWQQ_EC_WRONG_VERIFY;
      break;

   case 5:
      lwqq_log(LOG_ERROR, "Verify failed\n");
      err = LWQQ_EC_FAILD_VERIFY;
      break;

   case 6:
   case 71:
      lwqq_log(LOG_WARNING, "You may need to try login again\n");
      err = LWQQ_EC_ERROR;
      break;

   case 7:
      lwqq_log(LOG_ERROR, "Wrong input\n");
      err = LWQQ_EC_ERROR;
      break;

   case 8:
      lwqq_log(LOG_ERROR, "Too many logins on this IP. Please try again\n");
      err = LWQQ_EC_ERROR;
      break;

   case LWQQ_EC_LOGIN_NEED_BARCODE:
      lwqq_log(LOG_ERROR, "%s\n", msg);
      err = LWQQ_EC_LOGIN_NEED_BARCODE;
      break;

   default:
      err = LWQQ_EC_ERROR;
      lwqq_log(LOG_ERROR, "Unknow error");
      break;
   }

done:
   if(err)
      lwqq_override(lc->error_description, s_strdup(msg));
   lwqq_http_request_free(req);
   return err;
}

/** stage 5 **/
static LwqqAsyncEvent* check_sig(LwqqClient* lc, struct LoginStage* s)
{
   char* url = s->check_sig_url;
   LwqqHttpRequest* req
       = lwqq_http_create_default_request(lc, url, NULL);
   s->stage ++; // this is always success
   return req->do_request_async(
       req, lwqq__hasnot_post(), _C_(p_i, lwqq__process_empty, req));
}



static char* generate_clientid()
{
   int r;
   struct timeval tv;
   long t;
   char buf[20] = { 0 };

   srand(time(NULL));
   r = rand() % 90 + 10;
   if (gettimeofday(&tv, NULL)) {
      return NULL;
   }
   t = tv.tv_usec % 1000000;
   snprintf(buf, sizeof(buf), "%2d%6ld", r, t);
   return s_strdup(buf);
}

static int set_online_status_cb(LwqqHttpRequest* req, struct LoginStage* s);
/** staget 6 Set online status, this is the last step of login **/
static LwqqAsyncEvent* set_online_status(LwqqClient* lc, struct LoginStage* s) 
{
   char msg[1024] = { 0 };
   char* post = msg;
   LwqqHttpRequest* req = NULL;
   const char* status = lwqq_status_to_str(lc->stat);

   lwqq_override(lc->clientid, generate_clientid());

   /* Create a POST request */
   char url[512] = { 0 };
   snprintf(url, sizeof(url), "%s/channel/login2", WEBQQ_D_HOST);
   req = lwqq_http_create_default_request(lc, url, NULL);
   lwqq_http_debug(req, 5);

   lwqq_override(lc->session.ptwebqq, lwqq_http_get_cookie(req, "ptwebqq"));
   snprintf(msg, sizeof(msg),
            "r={\"status\":\"%s\",\"ptwebqq\":\"%s\","
            "\"passwd_sig\":"
            "\"\",\"clientid\":\"%s\""
            ", \"psessionid\":null}&clientid=%s&psessionid=null",
            status, lc->session.ptwebqq, lc->clientid, lc->clientid);
   urlencode(msg, 2);

   /* Set header needed by server */
   req->set_header(req, "Referer", WEBQQ_D_REF_URL);
   req->set_header(req, "Content-type", "application/x-www-form-urlencoded");
   return req->do_request_async(req, lwqq__has_post(),
                                _C_(2p_i, set_online_status_cb, req, s));
}

static int set_online_status_cb(LwqqHttpRequest* req, struct LoginStage* s)
{
   int err = 0;
   LwqqClient* lc = req->lc;
   json_t* root = NULL, *result;
   lwqq__jump_if_http_fail(req, err);
   lwqq__jump_if_json_fail(root, req->response, err);
   result = lwqq__parse_retcode_result(root, &err);
   if (err)
      goto done;
   if (result) {
      lwqq_override(lc->myself->uin, lwqq__json_get_value(result, "uin"));
      lwqq_override(lc->seskey, lwqq__json_get_value(result, "seskey"));
      lwqq_override(lc->cip, lwqq__json_get_value(result, "cip"));
      lwqq_override(lc->index, lwqq__json_get_value(result, "index"));
      lwqq_override(lc->port, lwqq__json_get_value(result, "port"));
      lwqq_override(lc->psessionid, lwqq__json_get_value(result, "psessionid"));
      lwqq_override(lc->vfwebqq, lwqq__json_get_value(result, "vfwebqq"));
      lc->stat
          = lwqq_status_from_str(json_parse_simple_value(result, "status"));
      s->stage ++;
   }
done:
   lwqq__log_if_error(err, req);
   lwqq__clean_json_and_req(root, req);
   return err;
}

LWQQ_EXPORT
LwqqAsyncEvent* lwqq_login(LwqqClient* lc, LwqqStatus status)
{
   if (!lc) {
      lwqq_log(LOG_ERROR, "Invalid pointer\n");
      return NULL;
   }
   if (status == LWQQ_STATUS_LOGOUT || status == LWQQ_STATUS_OFFLINE) {
      lwqq_log(LOG_WARNING, "Invalid online status\n");
      return NULL;
   }

   lc->stat = status;

   lc->args->login_ec = LWQQ_EC_NO_RESULT;
   vp_do_repeat(lc->events->start_login, NULL);

   struct LoginStage* s = s_malloc0(sizeof(*s));
   s->trigger = lwqq_async_event_new(NULL);
   s->trigger->lc = lc;
   s->stage = 0;
   do_login_stage(lc, s);
   return s->trigger;
}

/**
 * WebQQ logout function
 *
 * @param client Lwqq Client
 * @param err Error code
 */
LWQQ_EXPORT
LwqqErrorCode lwqq_logout(LwqqClient* lc, unsigned wait_time)
{
   vp_do_repeat(lc->events->start_logout, NULL);
   if(lc->stat == LWQQ_STATUS_LOGOUT) return LWQQ_EC_OK;
   lc->stat = LWQQ_STATUS_LOGOUT;
   LWQQ_SYNC_BEGIN(lc);
   lwqq_info_change_status(lc, LWQQ_STATUS_OFFLINE);
   LWQQ_SYNC_END(lc);
   return lc->sync_result;
}
#if 0
LWQQ_EXPORT
LwqqErrorCode lwqq_logout(LwqqClient *client, unsigned wait_time)
{
	LwqqClient* lc = client;
	LwqqHttpRequest *req = NULL;  
	int ret;
	LwqqErrorCode err = LWQQ_EC_OK;
	json_t *json = NULL;
	char *value;
	struct timeval tv;
	long int re;
	char url[512];

	if (!client) {
		lwqq_log(LOG_ERROR, "Invalid pointer\n");
		return LWQQ_EC_NULL_POINTER;
	}

	/* Get the milliseconds of now */
	if (gettimeofday(&tv, NULL)) {
		return LWQQ_EC_ERROR;
	}
	re = tv.tv_usec / 1000;
	re += tv.tv_sec;

	snprintf(url, sizeof(url), "%s/channel/logout2"
			"?clientid=%s&psessionid=%s&t=%ld",
			WEBQQ_D_HOST, client->clientid, client->psessionid, re);

	/* Create a GET request */
	req = lwqq_http_create_default_request(client,url, NULL);
	if (!req) {
		goto done;
	}

	/* Set header needed by server */
	req->set_header(req, "Referer", WEBQQ_LOGIN_REF_URL);

	if(wait_time>0)
		lwqq_http_set_option(req, LWQQ_HTTP_TIMEOUT, wait_time); // this perform good when no network link
	req->retry = 0;
	ret = req->do_request(req, 0, NULL);
	if (ret) {
		lwqq_log(LOG_ERROR, "Send logout request failed\n");
		err = LWQQ_EC_NETWORK_ERROR;
		goto done;
	}
	if (req->http_code != 200) {
		err = LWQQ_EC_HTTP_ERROR;
		goto done;
	}

	lwqq__jump_if_json_fail(json, req->response, err);

	/* Check whether logout correctly */
	value = json_parse_simple_value(json, "retcode");
	if (!value || strcmp(value, "0")) {
		err = LWQQ_EC_ERROR;
		goto done;
	}
	value = json_parse_simple_value(json, "result");
	if (!value || strcmp(value, "ok")) {
		err = LWQQ_EC_ERROR;
		goto done;
	}

done:
	lwqq__clean_json_and_req(json, req);
	client->stat = LWQQ_STATUS_LOGOUT;
	return err;
}
#endif

static int process_login2(LwqqHttpRequest* req)
{
   /*
    * {"retcode":0,"result":{"uin":2501542492,"cip":3396791469,"index":1075,"port":49648,"status":"online","vfwebqq":"8e6abfdb20f9436be07e652397a1197553f49fabd3e67fc88ad7ee4de763f337e120fdf7036176c9","psessionid":"8368046764001d636f6e6e7365727665725f77656271714031302e3133392e372e31363000003bce00000f8a026e04005c821a956d0000000a407646664c41737a42416d000000288e6abfdb20f9436be07e652397a1197553f49fabd3e67fc88ad7ee4de763f337e120fdf7036176c9","user_state":0,"f":0}}
    */
   int err = 0;
   LwqqClient* lc = req->lc;
   json_t* root = NULL, *result;
   lwqq__jump_if_http_fail(req, err);
   lwqq__jump_if_json_fail(root, req->response, err);
   result = lwqq__parse_retcode_result(root, &err);
   switch (err) {
   case 0:
      lwqq_puts("[ReLinkSuccess]");
      break;
   case 103:
      lwqq_puts("[Not Relogin]");
      vp_do_repeat(lc->events->poll_lost, NULL);
      goto done;
   case 113:
   case 115:
   case 112:
      lwqq_puts("[RelinkFailure]");
      vp_do_repeat(lc->events->poll_lost, NULL);
      goto done;
   default:
      lwqq_puts("[RelinkStop]");
      vp_do_repeat(lc->events->poll_lost, NULL);
      goto done;
   }
   if (result) {
      lwqq_override(lc->cip, lwqq__json_get_value(result, "cip"));
      lwqq_override(lc->index, lwqq__json_get_value(result, "index"));
      lwqq_override(lc->port, lwqq__json_get_value(result, "port"));
      lwqq_override(lc->psessionid, lwqq__json_get_value(result, "psessionid"));
      lwqq_override(lc->vfwebqq, lwqq__json_get_value(result, "vfwebqq"));
      lc->stat
          = lwqq_status_from_str(json_parse_simple_value(result, "status"));
   }
   lwqq_override(lc->session.ptwebqq, lwqq_http_get_cookie(req, "ptwebqq"));
done:
   lwqq__log_if_error(err, req);
   lwqq__clean_json_and_req(root, req);
   return err;
}

LWQQ_EXPORT
LwqqAsyncEvent* lwqq_relink(LwqqClient* lc)
{
   if (!lc)
      return NULL;
   char url[128];
   char post[512];
   snprintf(url, sizeof(url), "%s/channel/login2", WEBQQ_D_HOST);
   LwqqHttpRequest* req = lwqq_http_create_default_request(lc, url, NULL);
   // if(!lc->new_ptwebqq)
   //		lc->new_ptwebqq = lwqq_http_get_cookie(req, "ptwebqq");
   snprintf(post, sizeof(post), "r={"
                                "\"status\":\"%s\","
                                "\"ptwebqq\":\"%s\","
                                "\"passwd_sig\":\"\","
                                "\"clientid\":\"%s\","
                                "\"psessionid\":\"%s\"}",
            lwqq_status_to_str(lc->stat), lc->session.ptwebqq, lc->clientid,
            lc->psessionid);
   req->set_header(req, "Referer", WEBQQ_D_REF_URL);
   // lwqq_http_set_cookie(req, "ptwebqq", lc->new_ptwebqq, 1);
   req->retry = 0;
   return req->do_request_async(req, lwqq__has_post(),
                                _C_(p_i, process_login2, req));
}

static LoginFunc login_seq[] = {
  get_login_sig,
  check_need_verify,
  get_verify_image,
  do_login,
  check_sig,
  set_online_status,
  NULL
};

static void do_login_stage(LwqqClient* lc, struct LoginStage* s)
{
   LwqqErrorCode err = LWQQ_EC_OK;
   if (!lwqq_client_valid(lc))
      err = LWQQ_EC_ERROR;
   if (s->ev && s->ev->result != LWQQ_EC_OK){
      err = s->ev->result;
      if (err == LWQQ_EC_LOGIN_NEED_VC){
         // don't release resource, because, when user input vcode, we need
         // continue login progress
         s->ev = NULL;
         return;
      }
      goto onfail;
   }
   if(login_seq[s->stage] == NULL){
      err = LWQQ_EC_OK;
      goto done;
   }

   s->ev = login_seq[s->stage](lc, s);
   lwqq_async_add_event_listener(s->ev, _C_(2p, do_login_stage, lc, s));
   return;
onfail:
done:
   if(err) lc->stat = LWQQ_STATUS_LOGOUT;
   lc->args->login_ec = err;
   vp_do_repeat(lc->events->login_complete, NULL);
   s->trigger->result = err;
   lwqq_async_event_finish(s->trigger);
   s_free(s->vcode);
   s_free(s->salt);
   s_free(s->check_sig_url);
   s_free(s);
}
