/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <node_api.h>

#include <functional>
#include <memory>

namespace facebook::react {

/**
 * The host integration a Node-API engine needs from React Native: a worker
 * pool for napi_queue_async_work, dispatch onto the JS thread for thread-safe
 * functions, and handling of napi_fatal_exception. An engine adapter forwards
 * its embedder API's callbacks here.
 *
 * One instance exists per React Native runtime. Each env shares ownership of
 * it until the engine is done with the env, and queued async work until it
 * completes, so it's freed once the runtime and its work are gone.
 */
class NodeApiHost : public std::enable_shared_from_this<NodeApiHost> {
  // Restricts construction to create(), which shared_from_this relies on.
  struct PrivateTag {
    explicit PrivateTag() = default;
  };

 public:
  /**
   * Dispatches a function onto the JS thread, returning whether it was
   * accepted. Must be callable from any thread, must never run the function
   * inline, and must deliver accepted functions in order. Returning false is
   * only acceptable once the runtime is gone.
   */
  using JsDispatcher = std::function<bool(std::function<void()> &&)>;

  static std::shared_ptr<NodeApiHost> create(JsDispatcher dispatchToJs);

  NodeApiHost(PrivateTag, JsDispatcher dispatchToJs);

  bool dispatchToJs(std::function<void()> &&fn)
  {
    return dispatchToJs_(std::move(fn));
  }

  /**
   * Runs `execute` on a worker thread, then `complete` on the JS thread with
   * napi_ok.
   */
  void postWork(
      void *workData,
      void (*execute)(void *workData),
      void (*complete)(void *workData, napi_status status)) noexcept;

  /**
   * Removes work that hasn't started from the queue and completes it with
   * napi_cancelled, returning whether it did.
   */
  bool cancelWork(void *workData) noexcept;

  /**
   * Runs `callback` on the JS thread.
   */
  void postTask(void *taskData, void (*callback)(void *taskData)) noexcept;

  /**
   * Reports `error` through ErrorUtils.reportFatalError, aborting if that
   * fails. Runs synchronously on the JS thread.
   */
  void fatalException(napi_env env, napi_value error) noexcept;

  NodeApiHost(const NodeApiHost &) = delete;
  NodeApiHost &operator=(const NodeApiHost &) = delete;

 private:
  JsDispatcher dispatchToJs_;
  bool inFatalException_ = false;
};

} // namespace facebook::react
