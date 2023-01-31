# SPDX-License-Identifier: Apache-2.0
# Copyright 2023 The Foundry Visionmongers Ltd

# pylint: disable=no-member
# pylint: disable=wrong-import-position,unused-import
# pylint: disable=missing-function-docstring,missing-module-docstring

import os
import pytest

# This environment var must be set before the usd imports.
os.environ["TF_DEBUG"] = "OPENASSETIO_RESOLVER"
from pxr import Plug, Usd, Ar


# Assume OpenAssetIO is configured as the custom primary resolver for
# all tests. If you're wondering where this is configured, it may
# just be set via the `PXR_PLUGINPATH_NAME` environment variable.


# This test can be removed once the logging transforms, alchemy like,
# into real functionality.
def test_open_stage_and_logging(capfd):
    open_stage("resources/empty_shot.usda")
    captured = capfd.readouterr()

    outputs = captured.out.split(os.environ["TF_DEBUG"])
    assert "UsdOpenAssetIOResolver::UsdOpenAssetIOResolver" in outputs[1]
    assert "_CreateIdentifier" in outputs[2]
    assert "_Resolve" in outputs[3]
    assert "_GetExtension" in outputs[4]
    assert "_GetAssetInfo" in outputs[5]
    assert "_OpenAsset" in outputs[6]
    assert "_GetModificationTimestamp" in outputs[7]
    assert "_GetExtension" in outputs[8]


# Given a USD document that references an asset via a direct relative
# file path, then the asset is resolved to the file path as expected.
def test_openassetio_resolver_has_no_effect_with_no_search_path():

    stage = open_stage(
        "resources/integration_test_data/resolver_has_no_effect_with_no_search_path/parking_lot.usd"
    )

    assert_parking_lot_structure(stage)


# Given a USD document that references an asset via a relative
# search-path based file path, then the asset is resolved to the file
# path as expected.
def test_openassetio_resolver_has_no_effect_with_search_path():

    context = build_search_path_context(
        "resources/integration_test_data/resolver_has_no_effect_with_search_path/search_path_root"
    )
    stage = open_stage(
        "resources/integration_test_data/resolver_has_no_effect_with_search_path/parking_lot.usd",
        context,
    )

    assert_parking_lot_structure(stage)


# Given a USD document that references a second level document via an
# assetized reference resolvable by OpenAssetIO, and that second
# level document containing an assetized reference resolvable by
# OpenAssetIO to a third level document, and that the resolved paths
# are search path based, then the document can be fully resolved.
@pytest.mark.xfail(reason="OpenAssetIO not integrated yet")
def test_recursive_assetized_resolve():
    stage = open_stage(
        "resources/integration_test_data/recursive_assetized_resolve/parking_lot.usd"
    )

    assert_parking_lot_structure(stage)


# Given a USD document that references a second level document via an
# assetized reference resolvable by OpenAssetIO, and that second level
# document containing a non-assetized, adjacent relative file path
# reference to a third level document, then the document can be fully
# resolved.
@pytest.mark.xfail(reason="OpenAssetIO not integrated yet")
def test_assetized_child_ref_non_assetized_grandchild():
    stage = open_stage(
        "resources/integration_test_data"
        "/assetized_child_ref_non_assetized_grandchild/parking_lot.usd"
    )

    assert_parking_lot_structure(stage)


# Given a USD document that references a second level document via an
# non-assetized, adjacent relative file path reference, and that second
# level document containing an assetized reference resolvable by
# OpenAssetIO to a third level document, then the document can be fully
# resolved.
@pytest.mark.xfail(reason="OpenAssetIO not integrated yet")
def test_non_assetized_child_ref_assetized_grandchild():
    stage = open_stage(
        "resources/integration_test_data"
        "/non_assetized_child_ref_assetized_grandchild/parking_lot.usd"
    )

    assert_parking_lot_structure(stage)


##### Utility Functions #####

# Verify OpenAssetIO configured as the AR resolver.
@pytest.fixture(autouse=True)
def openassetio_configured():
    plugin_registry = Plug.Registry()
    plugin = plugin_registry.GetPluginWithName("usdOpenAssetIOResolver")

    assert (
        plugin is not None
    ), "usdOpenAssetIOResolver plugin not loaded, please check PXR_PLUGINPATH_NAME env variable"


# Log openassetio resolver messages
@pytest.fixture(autouse=True)
def enable_openassetio_logging_debug():
    os.environ["OPENASSETIO_LOGGING_SEVERITY"] = "1"


# As all the data tends to follow the same form, convenience method
# to avoid repeating myself
def assert_parking_lot_structure(usd_stage):
    floor = usd_stage.GetPrimAtPath("/ParkingLot/ParkingLot_Floor_1")

    assert floor.IsValid()

    car1 = floor.GetChild("Car1")
    car2 = floor.GetChild("Car2")

    assert car1.IsValid()
    assert car2.IsValid()

    assert car1.GetPropertyNames() == ["color"]
    assert car2.GetPropertyNames() == ["color"]


# Set the default resolver search path to a subdirectory
# Done this way so that you can run the test from any directory,
# otherwise the working directory will impact the file loading.
def build_search_path_context(path_relative_from_file):
    script_dir = os.path.realpath(os.path.dirname(__file__))
    full_path = os.path.join(script_dir, path_relative_from_file)
    return Ar.DefaultResolverContext([full_path])


# Open the stage
# Done this way so that you can run the test from any directory,
# otherwise the working directory will impact the file loading.
def open_stage(path_relative_from_file, context=None):
    script_dir = os.path.realpath(os.path.dirname(__file__))
    full_path = os.path.join(script_dir, path_relative_from_file)

    if context is not None:
        return Usd.Stage.Open(full_path, context)

    return Usd.Stage.Open(full_path)
