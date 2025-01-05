# Node-API tests

This directory contains tests to validate React Native's support for the Node API.

## Running the tests (iOS)

To run the tests, you need a version of Hermes that is compatible with the React Native version on this branch and includes Node API support. At the time of writing, [the PR adding Node-API support to Hermes](https://github.com/facebook/hermes/pull/1377) has not yet been merged.

First, run `pod install` pointing to the Hermes branch with Node API (based on `rn/0.77-stable`):

```
RCT_BUILD_HERMES_FROM_SOURCE=true HERMES_GITHUB_URL=https://github.com/kraenhansen/hermes.git HERMES_COMMIT=5b9416cdbade53fcc2f647fb0b41534fcd0701b9 pod install
```

> [!NOTE]  
> If the "pod install" command fails with the following error, delete all occurrences of "hermes-engine" in the Podfile.lock and try again:
> ```
> [Hermes] Using commit defined by HERMES_COMMIT envvar: 1692057b7b4669c4d48e00304f036de8ee95a65a
> [!] CocoaPods could not find compatible versions for pod "hermes-engine":
>   In snapshot (Podfile.lock):
>     hermes-engine (from `../react-native/sdks/hermes-engine/hermes-engine.podspec`)
> 
>   In Podfile:
>     hermes-engine (from `../react-native/sdks/hermes-engine/hermes-engine.podspec`)
```

## How it works

- A babel plugin transforming require statements for ".node" files.

