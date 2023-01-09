// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 The Foundry Visionmongers Ltd

#include "resolver.h"

#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_USING_DIRECTIVE
PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(UsdOpenAssetIOResolver, ArResolver);

TF_DEBUG_CODES(OAIO_RESOLVER);

PXR_NAMESPACE_CLOSE_SCOPE

// ------------------------------------------------------------
/* Ar Resolver Implementation */
UsdOpenAssetIOResolver::UsdOpenAssetIOResolver()
{
    TF_DEBUG(OAIO_RESOLVER).Msg("OAIO Resolver: " + TF_FUNC_NAME() + "\n");
}

UsdOpenAssetIOResolver::~UsdOpenAssetIOResolver()
{
    TF_DEBUG(OAIO_RESOLVER).Msg("OAIO Resolver: " + TF_FUNC_NAME() + "\n");
}

std::string UsdOpenAssetIOResolver::_CreateIdentifier(
    const std::string &assetPath,
    const ArResolvedPath &anchorAssetPath) const
{
    TF_DEBUG(OAIO_RESOLVER).Msg("OAIO Resolver: " + TF_FUNC_NAME() + "\n");
    return ArDefaultResolver::_CreateIdentifier(assetPath, anchorAssetPath);
}

std::string UsdOpenAssetIOResolver::_CreateIdentifierForNewAsset(
    const std::string &assetPath,
    const ArResolvedPath &anchorAssetPath) const
{
    TF_DEBUG(OAIO_RESOLVER).Msg("OAIO Resolver: " + TF_FUNC_NAME() + "\n");
    return ArDefaultResolver::_CreateIdentifierForNewAsset(assetPath, anchorAssetPath);
}

ArResolvedPath UsdOpenAssetIOResolver::_Resolve(
    const std::string &assetPath) const
{
    TF_DEBUG(OAIO_RESOLVER).Msg("OAIO Resolver: " + TF_FUNC_NAME() + "\n");
    return ArDefaultResolver::_Resolve(assetPath);
}

ArResolvedPath UsdOpenAssetIOResolver::_ResolveForNewAsset(
    const std::string &assetPath) const
{
    TF_DEBUG(OAIO_RESOLVER).Msg("OAIO Resolver: " + TF_FUNC_NAME() + "\n");
    return ArDefaultResolver::_ResolveForNewAsset(assetPath);
}

/* Asset Operations*/
std::string UsdOpenAssetIOResolver::_GetExtension(const std::string &assetPath) const
{
    TF_DEBUG(OAIO_RESOLVER).Msg("OAIO Resolver: " + TF_FUNC_NAME() + "\n");
    return ArDefaultResolver::_GetExtension(assetPath);
}

ArAssetInfo UsdOpenAssetIOResolver::_GetAssetInfo(
    const std::string &assetPath,
    const ArResolvedPath &resolvedPath) const
{
    TF_DEBUG(OAIO_RESOLVER).Msg("OAIO Resolver: " + TF_FUNC_NAME() + "\n");
    return ArDefaultResolver::_GetAssetInfo(assetPath, resolvedPath);
}

ArTimestamp UsdOpenAssetIOResolver::_GetModificationTimestamp(
    const std::string &assetPath,
    const ArResolvedPath &resolvedPath) const
{
    TF_DEBUG(OAIO_RESOLVER).Msg("OAIO Resolver: " + TF_FUNC_NAME() + "\n");
    return ArDefaultResolver::_GetModificationTimestamp(assetPath, resolvedPath);
}

std::shared_ptr<ArAsset> UsdOpenAssetIOResolver::_OpenAsset(
    const ArResolvedPath &resolvedPath) const
{
    TF_DEBUG(OAIO_RESOLVER).Msg("OAIO Resolver: " + TF_FUNC_NAME() + "\n");
    return ArDefaultResolver::_OpenAsset(resolvedPath);
}

bool UsdOpenAssetIOResolver::_CanWriteAssetToPath(const ArResolvedPath &resolvedPath,
                                                  std::string *whyNot) const
{
    TF_DEBUG(OAIO_RESOLVER).Msg("OAIO Resolver: " + TF_FUNC_NAME() + "\n");
    return ArDefaultResolver::CanWriteAssetToPath(resolvedPath, whyNot);
}

std::shared_ptr<ArWritableAsset> UsdOpenAssetIOResolver::_OpenAssetForWrite(
    const ArResolvedPath &resolvedPath,
    WriteMode writeMode) const
{
    TF_DEBUG(OAIO_RESOLVER).Msg("OAIO Resolver: " + TF_FUNC_NAME() + "\n");
    return ArDefaultResolver::_OpenAssetForWrite(resolvedPath, std::move(writeMode));
}
