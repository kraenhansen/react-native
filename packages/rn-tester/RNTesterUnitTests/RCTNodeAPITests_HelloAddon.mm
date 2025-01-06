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
#import <React-RuntimeHermes/react/runtime/hermes/HermesInstance.h>

#import <jsi/jsi.h>
#import <node_api.h>
#import <napi.h>

// TODO: Remove this declaration once its added to a header from hermes
NAPI_EXTERN napi_status NAPI_CDECL hermes_create_napi_env(
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

static facebook::jsi::Value value_from_napi_to_jsi(Napi::Env env, Napi::Value value, facebook::hermes::HermesRuntime& hermes_runtime) {
  // Store value as a global via Node-API
  env.Global().Set("__test-value", value);
  // Retrive the value via JSI
  return hermes_runtime.global().getProperty(hermes_runtime, "__test-value");
};

// https://github.com/nodejs/node-addon-examples/tree/main/src/1-getting-started/1_hello_world
@interface RCTNodeAPITests_HelloAddon : XCTestCase
@end

@implementation RCTNodeAPITests_HelloAddon

- (void)setUp {
}

- (void)tearDown {
}

- (void)testHelloAddon {
  ::hermes::vm::RuntimeConfig rt_config = ::hermes::vm::RuntimeConfig::Builder().build();
  auto hermes_runtime = facebook::hermes::makeHermesRuntime(rt_config);
  ::hermes::vm::Runtime* rt = hermes_runtime->getVMRuntimeUnsafe();
  
  napi_env _env;
  hermes_create_napi_env(*rt, false, nullptr, rt_config, &_env);
  Napi::Env env{_env};
  Napi::HandleScope {env};
  
  Napi::Object exports = Napi::Object::New(env);
  HelloAddon::Init(env, exports);
  
  // Test via Node-API
  auto result = exports
    .Get("hello")
    .As<Napi::Function>()
    .Call(exports, {});
  XCTAssertTrue(result.IsString());
  XCTAssertTrue(result.StrictEquals(Napi::String::New(env, "world")));
  
  // Test via JSI
  auto jsi_exports = value_from_napi_to_jsi(env, exports, *hermes_runtime)
    .asObject(*hermes_runtime);
  auto jsi_result = jsi_exports
    .getProperty(*hermes_runtime, "hello")
    .asObject(*hermes_runtime)
    .asFunction(*hermes_runtime)
    .callWithThis(*hermes_runtime, jsi_exports, {});
  XCTAssertTrue(jsi_result.isString());
  XCTAssertEqual(jsi_result.asString(*hermes_runtime).utf8(*hermes_runtime), std::string("world"));
}

@end
