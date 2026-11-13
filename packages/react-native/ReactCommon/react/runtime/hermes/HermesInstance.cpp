/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "HermesInstance.h"

#include <hermes/inspector-modern/chrome/HermesRuntimeTargetDelegate.h>
#include <jsi/jsilib.h>
#include <jsinspector-modern/InspectorFlags.h>
#include <logger/react_native_log.h>
#include <react/featureflags/ReactNativeFeatureFlags.h>

#ifdef RCT_NODE_API_ENABLED
#include "HermesNodeApiEngine.h"

#include <ReactCommon/CxxTurboModuleUtils.h>
#include <react/nativemodule/nodeapi/NativeNodeApi.h>
#endif

#include <mutex>

using namespace facebook::hermes;
using namespace facebook::jsi;

namespace facebook::react {

static void hermesFatalHandler(const std::string& reason) {
  react_native_log_fatal(reason.c_str());
}

static constinit std::once_flag fatalHandlerFlag;
#ifdef RCT_NODE_API_ENABLED
static constinit std::once_flag nodeApiFlag;
#endif

#ifdef RCT_NODE_API_ENABLED
// Owns a Hermes runtime together with state that must outlive it.
class HermesRuntimeOwner {
 public:
  HermesRuntimeOwner(
      std::unique_ptr<HermesRuntime> runtime,
      std::shared_ptr<void> outlivesRuntime)
      : runtime_(std::move(runtime)),
        outlivesRuntime_(std::move(outlivesRuntime)) {}

  ~HermesRuntimeOwner() {
    // Destroy the runtime first: it can use outlivesRuntime_ until it's gone.
    runtime_.reset();
  }

  HermesRuntimeOwner(const HermesRuntimeOwner&) = delete;
  HermesRuntimeOwner& operator=(const HermesRuntimeOwner&) = delete;

 private:
  std::unique_ptr<HermesRuntime> runtime_;
  std::shared_ptr<void> outlivesRuntime_;
};
#endif

class HermesJSRuntime : public JSRuntime {
 public:
  HermesJSRuntime(
      std::shared_ptr<jsi::Runtime> runtime,
      HermesRuntime& hermesRuntime)
      : runtime_(std::move(runtime)), hermesRuntime_(hermesRuntime) {}

  jsi::Runtime& getRuntime() noexcept override {
    return *runtime_;
  }

  jsinspector_modern::RuntimeTargetDelegate& getRuntimeTargetDelegate()
      override {
    if (!targetDelegate_) {
      targetDelegate_.emplace(runtime_, hermesRuntime_);
    }
    return *targetDelegate_;
  }

  void unstable_initializeOnJsThread() override {
    hermesRuntime_.registerForProfiling();
  }

 private:
  std::shared_ptr<jsi::Runtime> runtime_;
  HermesRuntime& hermesRuntime_;
  std::optional<jsinspector_modern::HermesRuntimeTargetDelegate>
      targetDelegate_;
};

std::unique_ptr<JSRuntime> HermesInstance::createJSRuntime(
    std::shared_ptr<::hermes::vm::CrashManager> crashManager,
    std::shared_ptr<MessageQueueThread> msgQueueThread,
    bool allocInOldGenBeforeTTI) noexcept {
  assert(msgQueueThread != nullptr);

  std::call_once(fatalHandlerFlag, []() {
    auto* fatalHandlerInterface =
        jsi::castInterface<ISetFatalHandler>(makeHermesRootAPI());
    if (fatalHandlerInterface) {
      fatalHandlerInterface->setFatalHandler(hermesFatalHandler);
    }
  });

#ifdef RCT_NODE_API_ENABLED
  std::call_once(nodeApiFlag, []() {
    registerCxxModuleToGlobalModuleMap(
        std::string(NativeNodeApi::kModuleName),
        [](std::shared_ptr<CallInvoker> jsInvoker) {
          return std::make_shared<NativeNodeApi>(
              std::make_shared<HermesNodeApiEngine>(), std::move(jsInvoker));
        });
  });
#endif

  auto gcConfig = ::hermes::vm::GCConfig::Builder()
                      // Default to 3GB
                      .withMaxHeapSize(3072 << 20)
                      .withName("RNBridgeless");

  if (allocInOldGenBeforeTTI) {
    // For the next two arguments: avoid GC before TTI
    // by initializing the runtime to allocate directly
    // in the old generation, but revert to normal
    // operation when we reach the (first) TTI point.
    gcConfig.withAllocInYoung(false).withRevertToYGAtTTI(true);
  }

  ::hermes::vm::RuntimeConfig::Builder runtimeConfigBuilder =
      ::hermes::vm::RuntimeConfig::Builder()
          .withGCConfig(gcConfig.build())
          .withEnableSampleProfiling(true)
          .withMicrotaskQueue(
              ReactNativeFeatureFlags::enableBridgelessArchitecture());

  if (crashManager) {
    runtimeConfigBuilder.withCrashMgr(crashManager);
  }

  std::unique_ptr<HermesRuntime> hermesRuntime =
      hermes::makeHermesRuntime(runtimeConfigBuilder.build());

  auto errorPrototype = hermesRuntime->global()
                            .getPropertyAsObject(*hermesRuntime, "Error")
                            .getPropertyAsObject(*hermesRuntime, "prototype");
  errorPrototype.setProperty(*hermesRuntime, "jsEngine", "hermes");

  (void)msgQueueThread;

  HermesRuntime& hermesRuntimeRef = *hermesRuntime;
#ifdef RCT_NODE_API_ENABLED
  // Hermes tears down Node-API envs, and uses their host, while the runtime is
  // destroyed. Every copy of the runtime pointer shares ownership of the owner,
  // so the host is freed once the last one is gone and the runtime with it.
  auto nodeApiState = HermesNodeApiEngine::attachToRuntime(hermesRuntimeRef);
  auto owner = std::make_shared<HermesRuntimeOwner>(
      std::move(hermesRuntime), std::move(nodeApiState));
  std::shared_ptr<jsi::Runtime> runtime(std::move(owner), &hermesRuntimeRef);
#else
  std::shared_ptr<jsi::Runtime> runtime = std::move(hermesRuntime);
#endif
  return std::make_unique<HermesJSRuntime>(
      std::move(runtime), hermesRuntimeRef);
}

} // namespace facebook::react
