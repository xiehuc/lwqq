#include "lwqq.h"
#include "lwjs.h"
#include "internal.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef WITH_MOZJS

#include <jsapi.h>

struct lwqq_js_t {
	JSRuntime* runtime;
	JSContext* context;
	JSObject*  global;
};

static JSClass global_class = { 
	"global", 
	JSCLASS_GLOBAL_FLAGS|JSCLASS_NEW_RESOLVE, 
	JS_PropertyStub, 
	JS_PropertyStub, 
	JS_PropertyStub, 
	JS_StrictPropertyStub, 
	JS_EnumerateStub, 
	JS_ResolveStub, 
	JS_ConvertStub, 
	NULL, 
	JSCLASS_NO_OPTIONAL_MEMBERS 
}; 

static void report_error(JSContext *cx,  const char *message, JSErrorReport *report)
{ 
	lwqq_verbose(3, "%s:%u:%s\n",
			report->filename ? report->filename : "<no filename>", 
			(unsigned int) report->lineno, 
			message); 
} 

LWQQ_EXPORT
lwqq_js_t* lwqq_js_init()
{
	lwqq_js_t* h = s_malloc0(sizeof(*h));
	h->runtime = JS_NewRuntime(8L*1024L*1024L);
	h->context = JS_NewContext(h->runtime, 16*1024);
	JS_SetOptions(h->context, 
			JSOPTION_VAROBJFIX|JSOPTION_COMPILE_N_GO|JSOPTION_NO_SCRIPT_RVAL);
	JS_SetErrorReporter(h->context, report_error);
#ifdef MOZJS_185
	h->global = JS_NewCompartmentAndGlobalObject(h->context, &global_class, NULL);
#else
	h->global = JS_NewGlobalObject(h->context,&global_class,NULL);
#endif
	JS_InitStandardClasses(h->context, h->global);
	return h;
}

LWQQ_EXPORT
lwqq_jso_t* lwqq_js_load(lwqq_js_t* js,const char* file)
{
	JSObject* global = JS_GetGlobalObject(js->context);
#ifdef MOZJS_185
	JSObject* script = JS_CompileFile(js->context, global, file);
#else
	JSScript* script = JS_CompileUTF8File(js->context,global,file);
#endif
	JS_ExecuteScript(js->context, global, script, NULL);
	return (lwqq_jso_t*)script;
}

LWQQ_EXPORT
void lwqq_js_load_buffer(lwqq_js_t* js,const char* content)
{
	JSObject* global = JS_GetGlobalObject(js->context);
	JS_EvaluateScript(js->context,global,content,strlen(content),NULL,0,NULL);
}

LWQQ_EXPORT
void lwqq_js_unload(lwqq_js_t* js,lwqq_jso_t* obj)
{
	//JS_DecompileScriptObject(js->context, obj, <#const char *name#>, <#uintN indent#>);
}

LWQQ_EXPORT
char* lwqq_js_hash(const char* uin,const char* ptwebqq,lwqq_js_t* js)
{
	JSObject* global = JS_GetGlobalObject(js->context);
	jsval res;
	jsval argv[2];
	char* res_;

	JSString* uin_ = JS_NewStringCopyZ(js->context, uin);
	JSString* ptwebqq_ = JS_NewStringCopyZ(js->context, ptwebqq);
	argv[0] = STRING_TO_JSVAL(uin_);
	argv[1] = STRING_TO_JSVAL(ptwebqq_);
	JS_CallFunctionName(js->context, global, "P", 2, argv, &res);

	res_ = JS_EncodeString(js->context,JSVAL_TO_STRING(res));
	if(!res_) return 0;
	char* ret = strdup(res_);
	JS_free(js->context,res_);

	return ret;
}

LWQQ_EXPORT
char* lwqq_js_enc_pwd(const char* pwd, const char* salt, const char* vcode, lwqq_js_t* js)
{
	JSObject* global = JS_GetGlobalObject(js->context);
	jsval res;
	jsval argv[3];
	char* res_;

	JSString* pwd_= JS_NewStringCopyZ(js->context, pwd);
	JSString* salt_= JS_NewStringCopyZ(js->context, salt);
	JSString* vcode_= JS_NewStringCopyZ(js->context, vcode);
	argv[0] = STRING_TO_JSVAL(pwd_);
	argv[1] = STRING_TO_JSVAL(salt_);
	argv[2] = STRING_TO_JSVAL(vcode_);
	JS_CallFunctionName(js->context, global, "encryption", 3, argv, &res);

	if(!res.s.payload.i32 && !res.s.payload.u32) return 0;
	res_ = JS_EncodeString(js->context,JSVAL_TO_STRING(res));
	if(!res_) return 0;
	char* ret = strdup(res_);
	JS_free(js->context,res_);

	return ret;
}

LWQQ_EXPORT
void lwqq_js_close(lwqq_js_t* js)
{
	JS_DestroyContext(js->context);
	JS_DestroyRuntime(js->runtime);
	JS_ShutDown();
	s_free(js);
}
#else

struct lwqq_js_t{
};

LWQQ_EXPORT
lwqq_js_t* lwqq_js_init()
{
	return NULL;
}

LWQQ_EXPORT
void lwqq_js_close(lwqq_js_t* js)
{
}
#endif


// vim: ts=3 sw=3 sts=3 noet
