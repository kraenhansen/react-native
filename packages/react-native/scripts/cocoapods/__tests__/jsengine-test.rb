# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

require "test/unit"
require_relative "../jsengine.rb"
require_relative "./test_utils/podSpy.rb"
require_relative "./test_utils/PodMock.rb"
require_relative "./test_utils/Open3Mock.rb"

class JSEngineTests < Test::Unit::TestCase

    :react_native_path

    def setup
        @react_native_path = "../.."
        podSpy_cleanUp()

    end

    def teardown
        ENV['HERMES_ENGINE_TARBALL_PATH'] = nil
        Open3.reset()
        Pod::Config.reset()
        Pod::UI.reset()
        podSpy_cleanUp()
        ENV['USE_HERMES'] = '1'
        ENV['CI'] = nil
    end

    # =============== #
    # TEST - setupJsc #
    # =============== #
    def test_setupJsc_installsPods
        # Arrange
        fabric_enabled = false

        # Act
        setup_jsc!(:react_native_path => @react_native_path, :fabric_enabled => fabric_enabled)

        # Assert
        assert_equal(2, $podInvocationCount)
        assert_equal("../../ReactCommon/jsi", $podInvocation["React-jsi"][:path])
        assert_equal("../../ReactCommon/jsc", $podInvocation["React-jsc"][:path])
    end

    def test_setupJsc_installsPods_installsFabricSubspecWhenFabricEnabled
        # Arrange
        fabric_enabled = true

        # Act
        setup_jsc!(:react_native_path => @react_native_path, :fabric_enabled => fabric_enabled)

        # Assert
        assert_equal(3, $podInvocationCount)
        assert_equal("../../ReactCommon/jsi", $podInvocation["React-jsi"][:path])
        assert_equal("../../ReactCommon/jsc", $podInvocation["React-jsc"][:path])
        assert_equal("../../ReactCommon/jsc", $podInvocation["React-jsc/Fabric"][:path])
    end

    # ================== #
    # TEST - setupHermes #
    # ================== #
    def test_setupHermes_installsPods
        # Act
        setup_hermes!(:react_native_path => @react_native_path)

        # Assert
        assert_equal(3, $podInvocationCount)
        assert_equal("../../ReactCommon/jsi", $podInvocation["React-jsi"][:path])
        hermes_engine_pod_invocation = $podInvocation["hermes-engine"]
        assert_equal("../../sdks/hermes-engine/hermes-engine.podspec", hermes_engine_pod_invocation[:podspec])
        assert_equal("", hermes_engine_pod_invocation[:tag])
        assert_equal("../../ReactCommon/hermes", $podInvocation["React-hermes"][:path])
    end

end
