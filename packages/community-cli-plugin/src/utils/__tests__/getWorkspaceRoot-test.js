/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict-local
 * @format
 * @oncall react_native
 */

const {getWorkspaceRoot} = require('../getWorkspaceRoot');
const fs = require('fs');
const os = require('os');
const path = require('path');

function createTempPackage(
  packageJson: {...},
  packagePath: string = fs.mkdtempSync(
    path.join(os.tmpdir(), 'rn-metro-config-test-'),
  ),
) {
  fs.mkdirSync(packagePath, {recursive: true});
  if (typeof packageJson === 'object') {
    fs.writeFileSync(
      path.join(packagePath, 'package.json'),
      JSON.stringify(packageJson),
      'utf8',
    );
  }
  return packagePath;
}

describe('getWorkspaceRoot', () => {
  test('returns null if not in a workspace', () => {
    const tempPackagePath = createTempPackage({
      name: 'my-app',
    });
    expect(getWorkspaceRoot(tempPackagePath)).toBe(null);
  });

  test('supports an NPM workspace', () => {
    const tempWorkspaceRootPath = createTempPackage({
      name: 'package-root',
      workspaces: ['packages/my-app', 'packages/my-lib'],
    });
    const tempPackagePath = createTempPackage(
      {
        name: 'my-app',
      },
      path.join(tempWorkspaceRootPath, 'packages', 'my-app'),
    );
    expect(getWorkspaceRoot(tempPackagePath)).toBe(tempWorkspaceRootPath);
  });

  test('supports a Yarn workspace', () => {
    const tempWorkspaceRootPath = createTempPackage({
      name: 'package-root',
      workspaces: ['packages/*'],
    });
    const tempPackagePath = createTempPackage(
      {
        name: 'my-app',
      },
      path.join(tempWorkspaceRootPath, 'packages', 'my-app'),
    );
    expect(getWorkspaceRoot(tempPackagePath)).toBe(tempWorkspaceRootPath);
  });

  test('supports a Yarn workspace (object style)', () => {
    const tempWorkspaceRootPath = createTempPackage({
      name: 'package-root',
      workspaces: {
        packages: ['packages/*'],
      },
    });
    const tempPackagePath = createTempPackage(
      {
        name: 'my-app',
      },
      path.join(tempWorkspaceRootPath, 'packages', 'my-app'),
    );
    expect(getWorkspaceRoot(tempPackagePath)).toBe(tempWorkspaceRootPath);
  });

  test('supports a pnpm workspace', () => {
    const tempWorkspaceRootPath = createTempPackage({
      name: 'package-root',
    });
    // Create the pnpm workspace configuration (see https://pnpm.io/pnpm-workspace_yaml)
    const workspacesConfig = 'packages: ["packages/*"]';
    fs.writeFileSync(
      path.join(tempWorkspaceRootPath, 'pnpm-workspace.yaml'),
      workspacesConfig,
      'utf8',
    );
    const tempPackagePath = createTempPackage(
      {
        name: 'my-app',
      },
      path.join(tempWorkspaceRootPath, 'packages', 'my-app'),
    );
    expect(getWorkspaceRoot(tempPackagePath)).toBe(tempWorkspaceRootPath);
  });

  test('supports a pnpm workspace exclusion', () => {
    const tempWorkspaceRootPath = createTempPackage({
      name: 'package-root',
    });
    // Create the pnpm workspace configuration (see https://pnpm.io/pnpm-workspace_yaml)
    const workspacesConfig = 'packages: ["packages/*", "!packages/*-app"]';
    fs.writeFileSync(
      path.join(tempWorkspaceRootPath, 'pnpm-workspace.yaml'),
      workspacesConfig,
      'utf8',
    );
    const tempPackagePath = createTempPackage(
      {
        name: 'my-app',
      },
      path.join(tempWorkspaceRootPath, 'packages', 'my-app'),
    );
    expect(getWorkspaceRoot(tempPackagePath)).toBe(null);
  });
});
