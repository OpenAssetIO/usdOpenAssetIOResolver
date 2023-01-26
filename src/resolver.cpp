// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 The Foundry Visionmongers Ltd

#include "resolver.h"

#include <utility>

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/defineResolver.h"

// NOLINTNEXTLINE
PXR_NAMESPACE_USING_DIRECTIVE
PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(UsdOpenAssetIOResolver, ArResolver)

TF_DEBUG_CODES(OPENASSETIO_RESOLVER)

PXR_NAMESPACE_CLOSE_SCOPE

// ------------------------------------------------------------
/* Ar Resolver Implementation */
UsdOpenAssetIOResolver::UsdOpenAssetIOResolver() {
  TF_DEBUG(OPENASSETIO_RESOLVER).Msg("OPENASSETIO_RESOLVER: " + TF_FUNC_NAME() + "\n");
}

UsdOpenAssetIOResolver::~UsdOpenAssetIOResolver() {
  TF_DEBUG(OPENASSETIO_RESOLVER).Msg("OPENASSETIO_RESOLVER: " + TF_FUNC_NAME() + "\n");
}

std::string UsdOpenAssetIOResolver::_CreateIdentifier(
    const std::string &assetPath, const ArResolvedPath &anchorAssetPath) const {
  auto result = ArDefaultResolver::_CreateIdentifier(assetPath, anchorAssetPath);
  TF_DEBUG(OPENASSETIO_RESOLVER)
      .Msg("OPENASSETIO_RESOLVER: " + TF_FUNC_NAME() + "\n  assetPath: " + assetPath +
           "\n  anchorAssetPath: " + anchorAssetPath.GetPathString() + "\n  result: " + result +
           "\n");
  return result;
}

std::string UsdOpenAssetIOResolver::_CreateIdentifierForNewAsset(
    const std::string &assetPath, const ArResolvedPath &anchorAssetPath) const {
  auto result = ArDefaultResolver::_CreateIdentifierForNewAsset(assetPath, anchorAssetPath);
  TF_DEBUG(OPENASSETIO_RESOLVER)
      .Msg("OPENASSETIO_RESOLVER: " + TF_FUNC_NAME() + "\n  assetPath: " + assetPath +
           "\n  anchorAssetPath: " + anchorAssetPath.GetPathString() + "\n  result: " + result +
           "\n");
  return result;
}

ArResolvedPath UsdOpenAssetIOResolver::_Resolve(const std::string &assetPath) const {
  auto result = ArDefaultResolver::_Resolve(assetPath);
  TF_DEBUG(OPENASSETIO_RESOLVER)
      .Msg("OPENASSETIO_RESOLVER: " + TF_FUNC_NAME() + "\n  assetPath: " + assetPath +
           "\n  result: " + result.GetPathString() + "\n");
  return result;
}

ArResolvedPath UsdOpenAssetIOResolver::_ResolveForNewAsset(const std::string &assetPath) const {
  auto result = ArDefaultResolver::_ResolveForNewAsset(assetPath);
  TF_DEBUG(OPENASSETIO_RESOLVER)
      .Msg("OPENASSETIO_RESOLVER: " + TF_FUNC_NAME() + "\n  assetPath: " + assetPath +
           "\n  result: " + result.GetPathString() + "\n");
  return result;
}

/* Asset Operations*/
std::string UsdOpenAssetIOResolver::_GetExtension(const std::string &assetPath) const {
  auto result = ArDefaultResolver::_GetExtension(assetPath);
  TF_DEBUG(OPENASSETIO_RESOLVER)
      .Msg("OPENASSETIO_RESOLVER: " + TF_FUNC_NAME() + "\n  assetPath: " + assetPath +
           "\n  result: " + result + "\n");
  return result;
}

ArAssetInfo UsdOpenAssetIOResolver::_GetAssetInfo(const std::string &assetPath,
                                                  const ArResolvedPath &resolvedPath) const {
  auto result = ArDefaultResolver::_GetAssetInfo(assetPath, resolvedPath);
  TF_DEBUG(OPENASSETIO_RESOLVER)
      .Msg("OPENASSETIO_RESOLVER: " + TF_FUNC_NAME() + "\n  assetPath: " + assetPath +
           "\n  resolvedPath :" + resolvedPath.GetPathString() + "\n  result(assetName): " +
           result.assetName + "\n  result(repoPath): " + result.repoPath + "\n");
  return result;
}

ArTimestamp UsdOpenAssetIOResolver::_GetModificationTimestamp(
    const std::string &assetPath, const ArResolvedPath &resolvedPath) const {
  auto result = ArDefaultResolver::_GetModificationTimestamp(assetPath, resolvedPath);
  TF_DEBUG(OPENASSETIO_RESOLVER)
      .Msg("OPENASSETIO_RESOLVER: " + TF_FUNC_NAME() + "\n  assetPath: " + assetPath +
           "\n  resolvedPath :" + resolvedPath.GetPathString() +
           "\n  result: " + std::to_string(result.GetTime()) + "\n");
  return result;
}

std::shared_ptr<ArAsset> UsdOpenAssetIOResolver::_OpenAsset(
    const ArResolvedPath &resolvedPath) const {
  TF_DEBUG(OPENASSETIO_RESOLVER)
      .Msg("OPENASSETIO_RESOLVER: " + TF_FUNC_NAME() +
           "\n  resolvedPath :" + resolvedPath.GetPathString() + "\n");
  return ArDefaultResolver::_OpenAsset(resolvedPath);
}

bool UsdOpenAssetIOResolver::_CanWriteAssetToPath(const ArResolvedPath &resolvedPath,
                                                  std::string *whyNot) const {
  auto result = ArDefaultResolver::CanWriteAssetToPath(resolvedPath, whyNot);
  TF_DEBUG(OPENASSETIO_RESOLVER)
      .Msg("OPENASSETIO_RESOLVER: " + TF_FUNC_NAME() +
           "\n  resolvedPath :" + resolvedPath.GetPathString() +
           "\n  result: " + std::to_string(static_cast<int>(result)) + "\n");
  return result;
}

std::shared_ptr<ArWritableAsset> UsdOpenAssetIOResolver::_OpenAssetForWrite(
    const ArResolvedPath &resolvedPath, WriteMode writeMode) const {
  TF_DEBUG(OPENASSETIO_RESOLVER)
      .Msg("OPENASSETIO_RESOLVER: " + TF_FUNC_NAME() +
           "\n  resolvedPath :" + resolvedPath.GetPathString() + "\n");
  return ArDefaultResolver::_OpenAssetForWrite(resolvedPath, writeMode);
}
