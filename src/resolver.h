// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 The Foundry Visionmongers Ltd
#pragma once

#include <memory>

#include <pxr/usd/ar/defaultResolver.h>

class UsdOpenAssetIOResolver final : public PXR_NS::ArDefaultResolver
{
public:
    UsdOpenAssetIOResolver();
    virtual ~UsdOpenAssetIOResolver();

protected:
    /* Ar Resolver Implementation */
    std::string _CreateIdentifier(
        const std::string &assetPath,
        const PXR_NS::ArResolvedPath &anchorAssetPath) const final;

    std::string _CreateIdentifierForNewAsset(
        const std::string &assetPath,
        const PXR_NS::ArResolvedPath &anchorAssetPath) const final;

    PXR_NS::ArResolvedPath _Resolve(const std::string &assetPath) const final;

    PXR_NS::ArResolvedPath _ResolveForNewAsset(const std::string &assetPath) const final;

    /* Asset Operations*/
    std::string _GetExtension(const std::string &assetPath) const final;

    PXR_NS::ArAssetInfo _GetAssetInfo(const std::string &assetPath,
                                      const PXR_NS::ArResolvedPath &resolvedPath) const final;

    PXR_NS::ArTimestamp _GetModificationTimestamp(const std::string &assetPath,
                                                  const PXR_NS::ArResolvedPath &resolvedPath) const final;

    std::shared_ptr<PXR_NS::ArAsset> _OpenAsset(const PXR_NS::ArResolvedPath &resolvedPath) const final;

    bool _CanWriteAssetToPath(const PXR_NS::ArResolvedPath &resolvedPath,
                              std::string *whyNot) const final;

    std::shared_ptr<PXR_NS::ArWritableAsset> _OpenAssetForWrite(
        const PXR_NS::ArResolvedPath &resolvedPath,
        WriteMode writeMode) const final;

private:
};
