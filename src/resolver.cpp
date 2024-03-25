// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 The Foundry Visionmongers Ltd

#include "resolver.h"

#include <stdexcept>
#include <thread>

#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/ar/assetInfo.h>
#include <pxr/usd/ar/defaultResolver.h>
#include <pxr/usd/ar/defineResolver.h>

#include <openassetio/Context.hpp>
#include <openassetio/access.hpp>
#include <openassetio/errors/exceptions.hpp>
#include <openassetio/hostApi/HostInterface.hpp>
#include <openassetio/hostApi/Manager.hpp>
#include <openassetio/hostApi/ManagerFactory.hpp>
#include <openassetio/hostApi/ManagerImplementationFactoryInterface.hpp>
#include <openassetio/log/LoggerInterface.hpp>
#include <openassetio/log/SeverityFilter.hpp>
#include <openassetio/python/hostApi.hpp>
#include <openassetio/trait/TraitsData.hpp>

#include <openassetio_mediacreation/traits/content/LocatableContentTrait.hpp>

// NOLINTNEXTLINE
PXR_NAMESPACE_USING_DIRECTIVE
PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(UsdOpenAssetIOResolver, ArResolver)

TF_DEBUG_CODES(OPENASSETIO_RESOLVER)

PXR_NAMESPACE_CLOSE_SCOPE

namespace {
/*
 * Replaces all occurrences of the search string in the subject with
 * the supplied replacement.
 */
void replaceAllInString(std::string &subject, const std::string &search,
                        const std::string &replace) {
  const size_t searchLength = search.length();
  const size_t replaceLength = replace.length();
  size_t pos = 0;
  while ((pos = subject.find(search, pos)) != std::string::npos) {
    subject.replace(pos, searchLength, replace);
    pos += replaceLength;
  }
}

/*
 * OpenAssetIO LoggerInterface implementation
 *
 * Converter logger from OpenAssetIO log framing to USD log outputs.
 */
class UsdOpenAssetIOResolverLogger : public openassetio::log::LoggerInterface {
 public:
  void log(Severity severity, const openassetio::Str &message) override {
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
class UsdOpenAssetIOHostInterface : public openassetio::hostApi::HostInterface {
 public:
  [[nodiscard]] openassetio::Identifier identifier() const override {
    return "org.openassetio.usdresolver";
  }

  [[nodiscard]] openassetio::Str displayName() const override {
    return "OpenAssetIO USD Resolver";
  }
};

/**
 * Retrieve the resolved file path of an entity reference.
 *
 * This will resolve the "locateableContent" trait of the entity and
 * return the "location" property of it, converted from a URL to a
 * file path.
 *
 * @param manager OpenAssetIO manager plugin wrapper.
 * @param context OpenAssetIO calling context.
 * @param entityReference The reference to resolve.
 * @return Resolved file path.
 */
std::string resolveToPath(const openassetio::hostApi::ManagerPtr &manager,
                          const openassetio::ContextConstPtr &context,
                          const openassetio::EntityReference &entityReference) {
  using openassetio::access::ResolveAccess;
  using openassetio_mediacreation::traits::content::LocatableContentTrait;

  // Resolve the locatable content trait, this will provide a URL
  // that points to the final content
  const openassetio::trait::TraitsDataPtr traitsData = manager->resolve(
      entityReference, {LocatableContentTrait::kId}, ResolveAccess::kRead, context);

  // OpenAssetIO is URL based, but we need a path, so check the
  // scheme and decode into a path

  static constexpr std::string_view kFileURLScheme{"file://"};
  static constexpr std::size_t kProtocolSize = kFileURLScheme.size();

  auto urlOpt = LocatableContentTrait(traitsData).getLocation();

  if (!urlOpt.has_value()) {
    std::string msg = "Location not found for entity: ";
    msg += entityReference.toString();
    throw std::runtime_error(msg);
  }

  auto url = *urlOpt;

  if (url.rfind(kFileURLScheme, 0) == openassetio::Str::npos) {
    std::string msg = "Only file URLs are supported: ";
    msg += url;
    throw std::runtime_error(msg);
  }

  // TODO(tc): Decode % escape sequences properly
  replaceAllInString(url, "%20", " ");
  return url.substr(kProtocolSize);
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
 * @param fn Callable instance.
 * @param logger OpenAssetIO logger to log exception messages.
 * @param name Name of function/process that caused the exception, to
 * append to log messages.
 * @return Result of callable if no exception occurred, otherwise a
 * default-constructed object.
 */
template <typename Fn>
auto catchAndLogExceptions(Fn &&fn, const openassetio::log::LoggerInterfacePtr &logger,
                           const std::string_view name) {
  try {
    return fn();
  } catch (const openassetio::errors::OpenAssetIOException &exc) {
    std::string msg = "OpenAssetIO error in ";
    msg += name;
    msg += ": ";
    msg += exc.what();
    logger->critical(msg);
  } catch (...) {
    std::string msg = "OpenAssetIO error in ";
    msg += name;
    msg += ": unknown non-exception type caught";
    logger->critical(msg);
  }
  return std::invoke_result_t<Fn>{};
}
}  // namespace

// ------------------------------------------------------------

/*
 * Ar Resolver Implementation
 */

UsdOpenAssetIOResolver::UsdOpenAssetIOResolver() {
  // Note: it is safe to throw exceptions from the constructor. USD will
  // error gracefully, unlike exceptions from other member functions,
  // which can cause a segfault/sigabrt.
  // TODO(DF): Add a test for exceptions happening here.

  logger_ =
      openassetio::log::SeverityFilter::make(std::make_shared<UsdOpenAssetIOResolverLogger>());
  auto managerImplementationFactory =
      openassetio::python::hostApi::createPythonPluginSystemManagerImplementationFactory(logger_);

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

UsdOpenAssetIOResolver::~UsdOpenAssetIOResolver() = default;

/*
 * Read
 */

std::string UsdOpenAssetIOResolver::_CreateIdentifier(
    const std::string &assetPath, const ArResolvedPath &anchorAssetPath) const {
  return catchAndLogExceptions(
      [&] {
        std::string identifier;

        if (manager_->isEntityReferenceString(assetPath)) {
          // If assetPath is an entity reference we must preserve it
          // unmodified as the "identifier", since it'll be passed to
          // subsequent member functions.  We assume it will (eventually)
          // resolve to an absolute path, making the anchorAssetPath redundant
          // (for now).
          // TODO(DF): Allow the manager to provide an identifier representing
          //  an "anchored" entity reference via getWithRelationship.
          identifier = assetPath;
        } else {
          identifier = ArDefaultResolver::_CreateIdentifier(assetPath, anchorAssetPath);
        }

        return identifier;
      },
      logger_, TF_FUNC_NAME());
}

ArResolvedPath UsdOpenAssetIOResolver::_Resolve(const std::string &assetPath) const {
  return catchAndLogExceptions(
      [&] {
        if (auto entityReference = manager_->createEntityReferenceIfValid(assetPath)) {
          return ArResolvedPath{resolveToPath(manager_, context_, *entityReference)};
        }
        return ArDefaultResolver::_Resolve(assetPath);
      },
      logger_, TF_FUNC_NAME());
}

/*
 * Write
 *
 * We don't currently support writes to OpenAssetIO entity references.
 * In order to call register when the ArAsset is closed, we'll need to
 * not resolve in _ResolveForNewAsset, and pass the entity reference
 * through.
 */

std::string UsdOpenAssetIOResolver::_CreateIdentifierForNewAsset(
    const std::string &assetPath, const ArResolvedPath &anchorAssetPath) const {
  if (manager_->isEntityReferenceString(assetPath)) {
    std::string message = "Writes to OpenAssetIO entity references are not currently supported ";
    message += assetPath;
    logger_->critical(message);
    return "";
  }
  return ArDefaultResolver::_CreateIdentifierForNewAsset(assetPath, anchorAssetPath);
}

ArResolvedPath UsdOpenAssetIOResolver::_ResolveForNewAsset(const std::string &assetPath) const {
  if (manager_->isEntityReferenceString(assetPath)) {
    std::string message = "Writes to OpenAssetIO entity references are not currently supported ";
    message += assetPath;
    logger_->critical(message);
    return ArResolvedPath("");
  }
  return ArDefaultResolver::_ResolveForNewAsset(assetPath);
}

/*
 * Pass-through asset operations
 *
 * These methods are simply relayed to the ArDefaultResolver. There may
 * be interest in fetching data for some of these from the manager, but
 * we don't have a real use case just yet. Doing so increases complexity
 * as we'd need to return both the resolved path _and_ the original
 * entity reference from _Resolve, so we could make queries in these
 * methods. We'll need this for publishing operations, but avoiding that
 * overhead for the more common (and hot) read case is critical.
 *
 */
std::string UsdOpenAssetIOResolver::_GetExtension(const std::string &assetPath) const {
  return ArDefaultResolver::_GetExtension(assetPath);
}

ArAssetInfo UsdOpenAssetIOResolver::_GetAssetInfo(const std::string &assetPath,
                                                  const ArResolvedPath &resolvedPath) const {
  return ArDefaultResolver::_GetAssetInfo(assetPath, resolvedPath);
}

ArTimestamp UsdOpenAssetIOResolver::_GetModificationTimestamp(
    const std::string &assetPath, const ArResolvedPath &resolvedPath) const {
  return ArDefaultResolver::_GetModificationTimestamp(assetPath, resolvedPath);
}

std::shared_ptr<ArAsset> UsdOpenAssetIOResolver::_OpenAsset(
    const ArResolvedPath &resolvedPath) const {
  return ArDefaultResolver::_OpenAsset(resolvedPath);
}

bool UsdOpenAssetIOResolver::_CanWriteAssetToPath(const ArResolvedPath &resolvedPath,
                                                  std::string *whyNot) const {
  return ArDefaultResolver::_CanWriteAssetToPath(resolvedPath, whyNot);
}

std::shared_ptr<ArWritableAsset> UsdOpenAssetIOResolver::_OpenAssetForWrite(
    const ArResolvedPath &resolvedPath, WriteMode writeMode) const {
  return ArDefaultResolver::_OpenAssetForWrite(resolvedPath, writeMode);
}
