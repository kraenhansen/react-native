/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifdef RCT_NODE_API_ENABLED

#include "HermesNodeApiEngine.h"

#include <jsi/hermes-interfaces.h>
#include <napi/hermes_napi.h>
#include <react/nativemodule/nodeapi/NodeApiHost.h>

#include <memory>
#include <vector>

namespace facebook::react {

namespace {

// The hermes_napi_host that a runtime's envs call back through, forwarding to
// a NodeApiHost.
class HermesHost {
 public:
  explicit HermesHost(std::shared_ptr<NodeApiHost> host)
      : host_(std::move(host)) {}

  // Hermes passes `this` back as the callbacks' `data`.
  HermesHost(const HermesHost&) = delete;
  HermesHost& operator=(const HermesHost&) = delete;

  const NodeApiHost* nodeApiHost() const {
    return host_.get();
  }

  hermes_napi_host callbacks{
      .post_work = &postWork,
      .cancel_work = &cancelWork,
      .post_task = &postTask,
      .data = this,
      .uv_loop = nullptr,
      .fatal_exception = &fatalException,
      // React Native has no event loop for thread-safe functions to keep
      // alive: the JS thread runs until the runtime is torn down.
      .ref_loop = nullptr,
      .unref_loop = nullptr,
  };

 private:
  static NodeApiHost& host(void* data) {
    return *static_cast<HermesHost*>(data)->host_;
  }

  static void postWork(
      void* data,
      void* workData,
      void (*execute)(void* workData),
      void (*complete)(void* workData, napi_status status)) noexcept {
    host(data).postWork(workData, execute, complete);
  }

  static bool cancelWork(void* data, void* workData) noexcept {
    return host(data).cancelWork(workData);
  }

  static void postTask(
      void* data,
      void* taskData,
      void (*callback)(void* taskData)) noexcept {
    host(data).postTask(taskData, callback);
  }

  static void
  fatalException(void* data, napi_env env, napi_value error) noexcept {
    host(data).fatalException(env, error);
  }

  std::shared_ptr<NodeApiHost> host_;
};

// A runtime's Node-API state. Its owner frees it after the runtime is
// destroyed, which destroys every env, so Hermes is done with the hosts by
// then.
class RuntimeState {
 public:
  hermes_napi_host& hostFor(std::shared_ptr<NodeApiHost> host) {
    // The runtime's NativeNodeApi instance normally provides the only host.
    for (const auto& hermesHost : hosts_) {
      if (hermesHost->nodeApiHost() == host.get()) {
        return hermesHost->callbacks;
      }
    }
    hosts_.push_back(std::make_unique<HermesHost>(std::move(host)));
    return hosts_.back()->callbacks;
  }

 private:
  std::vector<std::unique_ptr<HermesHost>> hosts_;
};

// Runtime data, freed at the start of the runtime's teardown, so it can only
// point at the state, not own it.
constexpr jsi::UUID kRuntimeStateUUID{
    0x3d2c0b97,
    0x3eec,
    0x46e2,
    0x8fb2,
    0x4506b2157b71};

std::shared_ptr<RuntimeState> getRuntimeState(jsi::Runtime& runtime) {
  auto data = runtime.getRuntimeData(kRuntimeStateUUID);
  if (!data) {
    return nullptr;
  }
  return std::static_pointer_cast<std::weak_ptr<RuntimeState>>(data)->lock();
}

} // namespace

std::shared_ptr<void> HermesNodeApiEngine::attachToRuntime(
    jsi::Runtime& runtime) {
  auto state = std::make_shared<RuntimeState>();
  runtime.setRuntimeData(
      kRuntimeStateUUID, std::make_shared<std::weak_ptr<RuntimeState>>(state));
  return state;
}

napi_env HermesNodeApiEngine::createEnv(
    jsi::Runtime& runtime,
    std::shared_ptr<NodeApiHost> host) {
  auto* hermes = jsi::castInterface<hermes::IHermes>(&runtime);
  auto state = getRuntimeState(runtime);
  if (hermes == nullptr || state == nullptr) {
    return nullptr;
  }
  return hermes_napi_create_env(
      hermes->getVMRuntimeUnsafe(), &state->hostFor(std::move(host)));
}

napi_status HermesNodeApiEngine::loadAddon(
    napi_env env,
    const char* filename,
    napi_value* result) {
  return hermes_napi_load_module(env, filename, result);
}

} // namespace facebook::react

#endif // RCT_NODE_API_ENABLED
