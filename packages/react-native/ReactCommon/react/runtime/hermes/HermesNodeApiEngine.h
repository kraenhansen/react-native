/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#ifdef RCT_NODE_API_ENABLED

#include <react/nativemodule/nodeapi/NodeApiEngine.h>

#include <memory>

namespace facebook::react {

class HermesNodeApiEngine : public NodeApiEngine {
 public:
  /**
   * Prepares `runtime` for Node-API addons, returning state the runtime's owner
   * must keep alive until after the runtime is destroyed: Hermes uses it while
   * it tears down the runtime's envs. createEnv fails for runtimes that weren't
   * prepared.
   */
  static std::shared_ptr<void> attachToRuntime(jsi::Runtime &runtime);

  napi_env createEnv(jsi::Runtime &runtime, std::shared_ptr<NodeApiHost> host) override;
  napi_status loadAddon(napi_env env, const char *filename, napi_value *result) override;
};

} // namespace facebook::react

#endif // RCT_NODE_API_ENABLED
