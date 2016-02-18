#include <node.h>
#include <node_version.h>
#include <v8.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libswitch_conf_ext.h"

using namespace std;
using namespace node;
using namespace v8;

//
// addon_isSwitchEnabled
// returns switch state as true or false
//
static Handle<Value> addon_isSwitchEnabled (const Arguments& args) {
	HandleScope scope;

	if (args.Length() != 3 || !args[0]->IsString() || !args[1]->IsString() || !args[2]->IsString()) {
		return ThrowException(Exception::TypeError(String::New("switch module, name and value missing")));
	}

	int active = libswitch_isSwitchEnabled(*String::Utf8Value(args[0]->ToString()), *String::Utf8Value(args[1]->ToString()), *String::Utf8Value(args[2]->ToString()));

	return scope.Close(Boolean::New(active?true:false));
}

struct init_data {
	Persistent<Function> cb;
	char configFile[512];
	int status;
};

static void addon_initializing (uv_work_t* uvWork) {
	init_data* initData = static_cast<init_data*> (uvWork->data);
	initData->status = libswitch_initializeLibSwitch(initData->configFile);
}

#if (NODE_MAJOR_VERSION == 0) && (NODE_MINOR_VERSION <= 8)
static void addon_initializationComplete (uv_work_t* uvWork) {
#else
static void addon_initializationComplete (uv_work_t* uvWork, int status) {
#endif
	init_data* initData = static_cast<init_data *> (uvWork->data);

	Local<Value> argv[1];

	if(initData->status==LS_SUCCESS) {
		argv[0] = Local<Value>::New(Null());
	}else{
		argv[0] = Local<Value>::New(String::New("initialization failed"));
	}

	/* Pass the argv array object to our callback function */
	TryCatch try_catch;

	initData->cb->Call(Context::GetCurrent()->Global(), 1, argv);
	if (try_catch.HasCaught()) {
		FatalException(try_catch);
	}

	// cleanup
	initData->cb.Dispose();

	free(uvWork);

	free(initData);
}



Handle<Value> addon_initialize(const Arguments& args) {
	HandleScope scope;

	if (args.Length() != 2 || !args[0]->IsString() || !args[1]->IsFunction()) {
		return ThrowException(Exception::TypeError(String::New("Usage: configFile, callback")));
	}
	Local<Function> cb = Local<Function>::Cast(args[1]);

	Local<Value> argv[1];

	init_data* rtd = (init_data*)malloc(sizeof(init_data));
	rtd->cb = Persistent<Function>::New(cb);
	strcpy(rtd->configFile, *String::Utf8Value(args[0]->ToString()));

	uv_work_t* uvWork = (uv_work_t*)malloc(sizeof(uv_work_t));
	uvWork->data = rtd;

	uv_queue_work(uv_default_loop(), uvWork, addon_initializing, addon_initializationComplete);

	return scope.Close(Undefined());
}

extern "C" void init(Handle<Object> target) {

	// initialize config.
	target->Set(String::NewSymbol("initialize"), FunctionTemplate::New(addon_initialize)->GetFunction());

	// set runtime hook for request processing
	target->Set(String::NewSymbol("isSwitchEnabled"), FunctionTemplate::New(addon_isSwitchEnabled)->GetFunction());

}

NODE_MODULE(switch_bindings, init)

