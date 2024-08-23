# SPDX-License-Identifier: Apache-2.0
# Copyright 2023 The Foundry Visionmongers Ltd
"""
Tests for the OpenAssetIO USD Ar2 adapter.

Note that OPENASSETIO_DEFAULT_CONFIG must point to a configuration file
that in turn points to a manager plugin that will satisfy these tests.
"""

# pylint: disable=no-member,redefined-outer-name
# pylint: disable=wrong-import-position,unused-import
# pylint: disable=missing-function-docstring,missing-module-docstring

import os
from unittest import mock

import pytest

# This environment var must be set before the usd imports.
os.environ["TF_DEBUG"] = "OPENASSETIO_RESOLVER"
if "PXR_PLUGINPATH_NAME" not in os.environ:
    # Set up our resolver plugin if not already set up, and we can
    root_dir = os.path.realpath(os.path.dirname(os.path.dirname(__file__)))
    plug_info = os.path.join(root_dir, "build", "dist", "resources", "pluginfo.json")
    if os.path.exists(plug_info):
        os.environ["PXR_PLUGINPATH_NAME"] = plug_info

from pxr import Plug, Usd, Ar, Sdf, Tf


# TODO(DF): More tests for error cases.


# Assert that the USD resolver checks that the OpenAssetIO manager is
# capable of `resolve`ing.
#
# Fork process so that the USD plugin is initialized again, but with
# patched `hasCapability`.
@pytest.mark.forked
def test_when_manager_cannot_resolve_then_exception_thrown(monkeypatch):
    # Use config that removes the "resolution" capability from the
    # manager's advertised set of capabilities.
    monkeypatch.setenv(
        "OPENASSETIO_DEFAULT_CONFIG",
        os.environ["OPENASSETIO_DEFAULT_CONFIG"] + ".incapable",
    )

    with pytest.raises(
        ValueError,
        match=".+ is not capable of resolving entity references",
    ):
        open_stage(
            "resources/integration_test_data/recursive_assetized_resolve/floors/floor 1.usd"
        )


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
def test_non_assetized_child_ref_assetized_grandchild():
    stage = open_stage(
        "resources/integration_test_data"
        "/non_assetized_child_ref_assetized_grandchild/parking_lot.usd"
    )

    assert_parking_lot_structure(stage)


# Will attempt to resolve `bal:///doesntexist` (i.e. a non-existent
# entity), which will trigger an exception in the manager internals.
def test_error_triggering_asset_ref(capfd):
    open_stage(
        "resources/integration_test_data/error_triggering_asset_ref/parking_lot.usd"
    )

    logs = capfd.readouterr()
    assert "entityResolutionError" in logs.err


def test_when_resolves_to_non_file_url_then_error():
    with pytest.raises(Tf.ErrorException) as exc:
        Usd.Stage.Open("bal:///not_a_file")
    # The exception seems to be truncated which looses the specifics of
    # our message (doh!). Check at least we were in the stack
    assert "UsdOpenAssetIOResolverLogger" in str(exc)


def test_when_writing_to_file_then_ok(capfd, tmp_path):
    Sdf.Layer.CreateNew(str(tmp_path / "newLayer.usda"))
    logs = capfd.readouterr()
    assert (
        "Writes to OpenAssetIO entity references are not currently supported"
        not in logs.err
    )


def test_when_writing_to_entity_ref_then_error():
    with pytest.raises(Tf.ErrorException) as exc:
        Sdf.Layer.CreateNew("bal:///newLayer")
    assert "Writes to OpenAssetIO entity references are not currently supported" in str(
        exc
    )


##### Utility Functions #####


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
