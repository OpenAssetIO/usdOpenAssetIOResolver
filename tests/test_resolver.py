# SPDX-License-Identifier: Apache-2.0
# Copyright 2023 The Foundry Visionmongers Ltd

# pylint: disable=no-member
# pylint: disable=wrong-import-position,unused-import
# pylint: disable=missing-function-docstring,missing-module-docstring

import os
import pytest

# This environment var must be set before the usd imports.
os.environ["TF_DEBUG"] = "OPENASSETIO_RESOLVER"
from pxr import Usd


def test_open_stage_and_logging(capfd):
    Usd.Stage.Open("resources/empty_shot.usda")
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
