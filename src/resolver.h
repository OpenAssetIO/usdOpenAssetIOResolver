// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 The Foundry Visionmongers Ltd
#pragma once

#include <memory>
#include <string>

#include <pxr/usd/ar/defaultResolver.h>

#include <openassetio/typedefs.hpp>

OPENASSETIO_FWD_DECLARE(hostApi, Manager)
OPENASSETIO_FWD_DECLARE(log, LoggerInterface)

class UsdOpenAssetIOResolver final : public PXR_NS::ArDefaultResolver {
 public:
  UsdOpenAssetIOResolver();
  ~UsdOpenAssetIOResolver() override;

 protected:
  /* Ar Resolver Implementation */
  [[nodiscard]] std::string _CreateIdentifier(
      const std::string &assetPath, const PXR_NS::ArResolvedPath &anchorAssetPath) const final;

  [[nodiscard]] std::string _CreateIdentifierForNewAsset(
      const std::string &assetPath, const PXR_NS::ArResolvedPath &anchorAssetPath) const final;

  [[nodiscard]] PXR_NS::ArResolvedPath _Resolve(const std::string &assetPath) const final;

  [[nodiscard]] PXR_NS::ArResolvedPath _ResolveForNewAsset(
      const std::string &assetPath) const final;

  /* Asset Operations*/
  [[nodiscard]] std::string _GetExtension(const std::string &assetPath) const final;

  [[nodiscard]] PXR_NS::ArAssetInfo _GetAssetInfo(
      const std::string &assetPath, const PXR_NS::ArResolvedPath &resolvedPath) const final;

  [[nodiscard]] PXR_NS::ArTimestamp _GetModificationTimestamp(
      const std::string &assetPath, const PXR_NS::ArResolvedPath &resolvedPath) const final;

  [[nodiscard]] std::shared_ptr<PXR_NS::ArAsset> _OpenAsset(
      const PXR_NS::ArResolvedPath &resolvedPath) const final;

  [[nodiscard]] bool _CanWriteAssetToPath(const PXR_NS::ArResolvedPath &resolvedPath,
                                          std::string *whyNot) const final;

  [[nodiscard]] std::shared_ptr<PXR_NS::ArWritableAsset> _OpenAssetForWrite(
      const PXR_NS::ArResolvedPath &resolvedPath, WriteMode writeMode) const final;

 private:
  openassetio::log::LoggerInterfacePtr logger_;
  openassetio::hostApi::ManagerPtr manager_;
};
