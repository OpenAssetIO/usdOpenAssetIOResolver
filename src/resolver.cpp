// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 The Foundry Visionmongers Ltd

#include <stdexcept>

#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/ar/assetInfo.h>
#include <pxr/usd/ar/defaultResolver.h>
#include <pxr/usd/ar/defineResolver.h>

#include <openassetio/Context.hpp>
#include <openassetio/access.hpp>
#include <openassetio/hostApi/HostInterface.hpp>
#include <openassetio/hostApi/Manager.hpp>
#include <openassetio/hostApi/ManagerFactory.hpp>
#include <openassetio/log/LoggerInterface.hpp>
#include <openassetio/log/SeverityFilter.hpp>
#include <openassetio/python/hostApi.hpp>
#include <openassetio/utils/path.hpp>

#include <openassetio_mediacreation/traits/content/LocatableContentTrait.hpp>

// NOLINTNEXTLINE
PXR_NAMESPACE_USING_DIRECTIVE
PXR_NAMESPACE_OPEN_SCOPE
TF_DEBUG_CODES(OPENASSETIO_RESOLVER)
PXR_NAMESPACE_CLOSE_SCOPE

namespace {
/*
 * OpenAssetIO LoggerInterface implementation
 *
 * Converter logger from OpenAssetIO log framing to USD log outputs.
 */
class UsdOpenAssetIOResolverLogger final : public openassetio::log::LoggerInterface {
 public:
  void log(const Severity severity, const openassetio::Str &message) override {
    switch (severity) {
      case Severity::kCritical:
        TF_ERROR(TfDiagnosticType::TF_DIAGNOSTIC_FATAL_ERROR_TYPE, message);
        break;
      case Severity::kDebug:
      case Severity::kDebugApi:
        TF_DEBUG(OPENASSETIO_RESOLVER).Msg(message + "\n");
        break;
      case Severity::kError:
        TF_ERROR(TfDiagnosticType::TF_DIAGNOSTIC_NONFATAL_ERROR_TYPE, message);
        break;
      case Severity::kInfo:
      case Severity::kProgress:
        TF_INFO(OPENASSETIO_RESOLVER).Msg(message + "\n");
        break;
      case Severity::kWarning:
        TF_WARN(TfDiagnosticType::TF_DIAGNOSTIC_WARNING_TYPE, message);
        break;
    }
  }
};

/**
 * OpenAssetIO HostInterface implementation
 *
 * This identifies the Ar2 plugin uniquely should the manager plugin
 * wish to adapt its behaviour.
 */
class UsdOpenAssetIOHostInterface final : public openassetio::hostApi::HostInterface {
 public:
  [[nodiscard]] openassetio::Identifier identifier() const override {
    return "org.openassetio.usdresolver";
  }

  [[nodiscard]] openassetio::Str displayName() const override {
    return "OpenAssetIO USD Resolver";
  }
};

}  // namespace

// ---------------------------------------------------------------------

/*
 * Ar Resolver Implementation
 */
class UsdOpenAssetIOResolver final : public PXR_NS::ArDefaultResolver {
 public:
  UsdOpenAssetIOResolver() {
    logger_ =
        openassetio::log::SeverityFilter::make(std::make_shared<UsdOpenAssetIOResolverLogger>());
    const auto managerImplementationFactory =
        openassetio::python::hostApi::createPythonPluginSystemManagerImplementationFactory(
            logger_);
    const auto hostInterface = std::make_shared<UsdOpenAssetIOHostInterface>();
    manager_ = openassetio::hostApi::ManagerFactory::defaultManagerForInterface(
        hostInterface, managerImplementationFactory, logger_);
    if (!manager_) {
      throw std::invalid_argument{
          "No default manager configured, " +
          openassetio::hostApi::ManagerFactory::kDefaultManagerConfigEnvVarName};
    }
    if (!manager_->hasCapability(openassetio::hostApi::Manager::Capability::kResolution)) {
      throw std::invalid_argument{manager_->displayName() +
                                  " is not capable of resolving entity references"};
    }
    context_ = openassetio::Context::make();
  }

  ~UsdOpenAssetIOResolver() override = default;

 protected:
  [[nodiscard]] std::string _CreateIdentifier(
      const std::string &assetPath, const ArResolvedPath &anchorAssetPath) const override {
    return catchAndLogExceptions(
        [&] {
          std::string identifier;
          if (manager_->isEntityReferenceString(assetPath)) {
            identifier = assetPath;
          } else {
            identifier = ArDefaultResolver::_CreateIdentifier(assetPath, anchorAssetPath);
          }
          return identifier;
        },
        TF_FUNC_NAME());
  }

  [[nodiscard]] ArResolvedPath _Resolve(const std::string &assetPath) const override {
    return catchAndLogExceptions(
        [&] {
          if (const auto entityReference = manager_->createEntityReferenceIfValid(assetPath)) {
            return ArResolvedPath{resolveToPath(*entityReference)};
          }
          return ArDefaultResolver::_Resolve(assetPath);
        },
        TF_FUNC_NAME());
  }

  [[nodiscard]] std::string _CreateIdentifierForNewAsset(
      const std::string &assetPath, const ArResolvedPath &anchorAssetPath) const override {
    if (manager_->isEntityReferenceString(assetPath)) {
      std::string message = "Writes to OpenAssetIO entity references are not currently supported ";
      message += assetPath;
      logger_->critical(message);
      return "";
    }
    return ArDefaultResolver::_CreateIdentifierForNewAsset(assetPath, anchorAssetPath);
  }

  [[nodiscard]] ArResolvedPath _ResolveForNewAsset(const std::string &assetPath) const override {
    if (manager_->isEntityReferenceString(assetPath)) {
      std::string message = "Writes to OpenAssetIO entity references are not currently supported ";
      message += assetPath;
      logger_->critical(message);
      return ArResolvedPath("");
    }
    return ArDefaultResolver::_ResolveForNewAsset(assetPath);
  }

  [[nodiscard]] std::string _GetExtension(const std::string &assetPath) const override {
    return ArDefaultResolver::_GetExtension(assetPath);
  }

  [[nodiscard]] ArAssetInfo _GetAssetInfo(const std::string &assetPath,
                                          const ArResolvedPath &resolvedPath) const override {
    return ArDefaultResolver::_GetAssetInfo(assetPath, resolvedPath);
  }

  [[nodiscard]] ArTimestamp _GetModificationTimestamp(
      const std::string &assetPath, const ArResolvedPath &resolvedPath) const override {
    return ArDefaultResolver::_GetModificationTimestamp(assetPath, resolvedPath);
  }

  [[nodiscard]] std::shared_ptr<ArAsset> _OpenAsset(
      const ArResolvedPath &resolvedPath) const override {
    return ArDefaultResolver::_OpenAsset(resolvedPath);
  }

  [[nodiscard]] bool _CanWriteAssetToPath(const ArResolvedPath &resolvedPath,
                                          std::string *whyNot) const override {
    return ArDefaultResolver::_CanWriteAssetToPath(resolvedPath, whyNot);
  }

  [[nodiscard]] std::shared_ptr<ArWritableAsset> _OpenAssetForWrite(
      const ArResolvedPath &resolvedPath, const WriteMode writeMode) const override {
    return ArDefaultResolver::_OpenAssetForWrite(resolvedPath, writeMode);
  }

 private:
  /**
   * Retrieve the resolved file path of an entity reference.
   *
   * This will resolve the "locateableContent" trait of the entity and
   * return the "location" property of it, converted from a URL to a
   * file path.
   *
   * @param entityReference The reference to resolve.
   * @return Resolved file path.
   */
  [[nodiscard]] std::string resolveToPath(
      const openassetio::EntityReference &entityReference) const {
    using openassetio::access::ResolveAccess;
    using openassetio_mediacreation::traits::content::LocatableContentTrait;

    // Resolve the locatable content trait, this will provide a URL
    // that points to the final content
    const openassetio::trait::TraitsDataPtr traitsData = manager_->resolve(
        entityReference, {LocatableContentTrait::kId}, ResolveAccess::kRead, context_);

    const std::optional<openassetio::Str> url = LocatableContentTrait(traitsData).getLocation();
    if (!url) {
      throw std::invalid_argument{"Entity reference does not have a location: " +
                                  entityReference.toString()};
    }
    // OpenAssetIO is URL based, but we need a path. Note: will throw if
    // the URL is not valid.
    return fileUrlPathConverter_.pathFromUrl(*url);
  }

  /**
   * Decorator to stop propagation of all exceptions.
   *
   * Exceptions occurring within the wrapped callable will be caught and
   * logged, and a default-constructed value (of the same type as the
   * callable's return type) returned instead of propagating the
   * exception.
   *
   * The callable must take no arguments (but may return a value).
   *
   * This is needed since USD reacts badly if an exception escapes an Ar
   * plugin (segfault, sigabrt).
   *
   * @tparam Fn Callable type.
   * @param func Callable instance.
   * @param name Name of function/process that caused the exception, to
   * append to log messages.
   * @return Result of callable if no exception occurred, otherwise a
   * default-constructed object.
   */
  template <typename Fn>
  auto catchAndLogExceptions(Fn &&func, const std::string_view name) const
      -> std::invoke_result_t<Fn> {
    try {
      return func();
    } catch (const std::exception &exc) {
      std::string msg = "OpenAssetIO error in ";
      msg += name;
      msg += ": ";
      msg += exc.what();
      logger_->critical(msg);
    } catch (...) {
      std::string msg = "OpenAssetIO error in ";
      msg += name;
      msg += ": unknown non-exception type caught";
      logger_->critical(msg);
    }
    return {};
  }

  openassetio::log::LoggerInterfacePtr logger_;
  openassetio::hostApi::ManagerPtr manager_;
  openassetio::ContextConstPtr context_;
  openassetio::utils::FileUrlPathConverter fileUrlPathConverter_;
};

PXR_NAMESPACE_OPEN_SCOPE
AR_DEFINE_RESOLVER(UsdOpenAssetIOResolver, ArResolver)
PXR_NAMESPACE_CLOSE_SCOPE
