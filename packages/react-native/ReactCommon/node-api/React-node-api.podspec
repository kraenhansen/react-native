# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

require "json"

use_hermes = ENV['USE_HERMES'] == nil || ENV['USE_HERMES'] == '1'

package = JSON.parse(File.read(File.join(__dir__, "..", "..", "package.json")))
version = package['version']

source = { :git => 'https://github.com/facebook/react-native.git' }
if version == '1000.0.0'
  # This is an unpublished version, use the latest commit hash of the react-native repo, which we’re presumably in.
  source[:commit] = `git rev-parse HEAD`.strip if system("git rev-parse --git-dir > /dev/null 2>&1")
else
  source[:tag] = "v#{version}"
end

Pod::Spec.new do |s|
  s.name                   = "React-node-api"
  s.version                = version
  s.summary                = "Node API runtime layer for React Native"
  s.homepage               = "https://reactnative.dev/"
  s.license                = package["license"]
  s.author                 = "Meta Platforms, Inc. and its affiliates"
  s.platforms              = min_supported_versions
  s.source                 = source

  s.header_dir    = "node-api"
  s.pod_target_xcconfig    = {
    "HEADER_SEARCH_PATHS" => "",
    "CLANG_CXX_LANGUAGE_STANDARD" => rct_cxx_language_standard(),
    "DEFINES_MODULE" => "YES"
  }

  s.source_files  = "**/*.{cpp,h}"
  s.dependency "React-jsi"
  s.exclude_files = [
    "**/test/*"
   ]
end
