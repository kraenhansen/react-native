//
//  RCTNodeAPITests.m
//  RNTesterUnitTests
//
//  Created by Kræn Hansen on 28/12/2024.
//  Copyright © 2024 Facebook. All rights reserved.
//

#import <XCTest/XCTest.h>

#import <hermes/hermes.h>
#import <hermes/Public/RuntimeConfig.h>
#import <hermes/ScriptStore.h>

#import <node_api.h>
#import <napi.h>

// TODO: Remove this declaration once its added to a header from hermes
extern "C" NAPI_EXTERN napi_status NAPI_CDECL hermes_create_napi_env(
    ::hermes::vm::Runtime& runtime,
    bool isInspectable,
    std::shared_ptr<facebook::jsi::PreparedScriptStore> preparedScript,
    const ::hermes::vm::RuntimeConfig& runtimeConfig,
    napi_env* env);


class HelloAddon : public Napi::Addon<HelloAddon> {
 public:
  HelloAddon(Napi::Env, Napi::Object exports) {
    DefineAddon(exports, {InstanceMethod("hello", &HelloAddon::Hello, napi_enumerable)});
  }

 private:
  Napi::Value Hello(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), "world");
  }
};

// https://github.com/nodejs/node-addon-examples/tree/main/src/1-getting-started/1_hello_world
@interface RCTNodeAPITests_HelloAddon : XCTestCase
@end

@implementation RCTNodeAPITests_HelloAddon

- (void)setUp {
}

- (void)tearDown {
}

- (void)testExample {
  ::hermes::vm::RuntimeConfig rt_config{};
  auto hermesRuntime = facebook::hermes::makeHermesRuntime(rt_config);
  ::hermes::vm::Runtime* rt = hermesRuntime->getVMRuntimeUnsafe();

  napi_env _env;
  hermes_create_napi_env(*rt, false, nullptr, rt_config, &_env);
  
  napi_value _exports = nullptr;
  napi_status status = napi_create_object(_env, &_exports);
  if (status != napi_ok) {
    throw std::runtime_error("Failed to build napi environment from hermes runtime");
  }
  Napi::Env env{_env};
  Napi::Object exports{_env, _exports};
  HelloAddon addon{env, exports};
  
  // TODO: Convert the Node Addon "exports" object back to JSI and call the "hello" function on it
}

@end
