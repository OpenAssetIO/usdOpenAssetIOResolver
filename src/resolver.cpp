// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 The Foundry Visionmongers Ltd

#include "resolver.h"

#include <sstream>
#include <thread>
#include <utility>

#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/ar/assetInfo.h>
#include <pxr/usd/ar/defaultResolver.h>
#include <pxr/usd/ar/defineResolver.h>

#include <openassetio/Context.hpp>
#include <openassetio/TraitsData.hpp>
#include <openassetio/hostApi/HostInterface.hpp>
#include <openassetio/hostApi/Manager.hpp>
#include <openassetio/hostApi/ManagerFactory.hpp>
#include <openassetio/hostApi/ManagerImplementationFactoryInterface.hpp>
#include <openassetio/log/LoggerInterface.hpp>
#include <openassetio/log/SeverityFilter.hpp>
#include <openassetio/python/hostApi.hpp>

// NOLINTNEXTLINE
PXR_NAMESPACE_USING_DIRECTIVE
PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(UsdOpenAssetIOResolver, ArResolver)

TF_DEBUG_CODES(OPENASSETIO_RESOLVER)

PXR_NAMESPACE_CLOSE_SCOPE

namespace {
/// Converter logger from OpenAssetIO log framing to USD log outputs.
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
        // TODO(EM) : Review to see which error types are most appropriate,
        //  are all errors (not criticals) non fatal?
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

class UsdOpenAssetIOHostInterface : public openassetio::hostApi::HostInterface {
 public:
  [[nodiscard]] openassetio::Identifier identifier() const override {
    return "org.openassetio.usdresolver";
  }

  [[nodiscard]] openassetio::Str displayName() const override {
    return "OpenAssetIO USD Resolver";
  }
};

// TODO(DF): Replace with C++ trait views, once they exist.
const openassetio::trait::TraitId kLocateableContentTraitId =  // NOLINT
    "openassetio-mediacreation:content.LocatableContent";
const openassetio::trait::property::Key kLocateableContentLocationPropertyKey =  // NOLINT
    "location";

/**
 * Retrieve the resolved file path of an entity reference.
 *
 * If the given `assetPath` is not an entity reference, assume it's
 * already a path and pass through unmodified.
 *
 * Otherwise, resolve the "locateableContent" trait of the entity and
 * return the "location" property of it, converted from a URL to a
 * posix file path.
 *
 * @param manager OpenAssetIO manager plugin wrapper.
 * @param context OpenAssetIO calling context.
 * @param assetPath String-like asset path that may or may not be an
 * entity reference.
 * @return Resolved file path if `assetPath` is an entity reference,
 * `assetPath` otherwise.
 */
template <typename Ref>
Ref locationInManagerContextForEntity(const openassetio::hostApi::ManagerPtr &manager,
                                      const openassetio::ContextConstPtr &context, Ref assetPath) {
  // Check if the assetPath is an OpenAssetIO entity reference.
  if (auto maybeEntityReference = manager->createEntityReferenceIfValid(assetPath)) {
    openassetio::TraitsDataPtr traitsData;

    // Resolve the locateableContent trait in order to get the
    // (absolute) path to the asset.
    manager->resolve(
        {std::move(*maybeEntityReference)}, {kLocateableContentTraitId}, context,
        [&traitsData]([[maybe_unused]] std::size_t idx,
                      const openassetio::TraitsDataPtr &resolvedTraitsData) {
          // Success callback.
          traitsData = resolvedTraitsData;
        },
        []([[maybe_unused]] std::size_t idx, const openassetio::BatchElementError &error) {
          // Error callback.
          // TODO(DF): Better conversion of BatchElementError to
          //  appropriate exception type.
          std::string errorMsg = "error code ";
          errorMsg += std::to_string(static_cast<int>(error.code));
          errorMsg += ": ";
          errorMsg += error.message;
          throw std::runtime_error{errorMsg};
        });

    if (openassetio::trait::property::Value propValue; traitsData->getTraitProperty(
            &propValue, kLocateableContentTraitId, kLocateableContentLocationPropertyKey)) {
      // We've successfully got the locateableContent trait for the
      // entity.
      static constexpr std::size_t kProtocolSize = std::string_view{"file://"}.size();
      return Ref{std::get<openassetio::Str>(propValue).substr(kProtocolSize)};
    }
    // TODO(DF): Here we fall through to returning `assetPath` verbatim
    //  if the "locateableContent" trait isn't found. We need to
    //  consider if this is the correct thing to do.
  }

  return assetPath;
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
  } catch (const std::exception &exc) {
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
/* Ar Resolver Implementation */
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

  // TODO(DF): Set appropriate locale.
  readContext_ = openassetio::Context::make(openassetio::Context::Access::kRead,
                                            openassetio::Context::Retention::kTransient);

  logger_->debug(TF_FUNC_NAME());
}

UsdOpenAssetIOResolver::~UsdOpenAssetIOResolver() { logger_->debug(TF_FUNC_NAME()); }

std::string UsdOpenAssetIOResolver::_CreateIdentifier(
    const std::string &assetPath, const ArResolvedPath &anchorAssetPath) const {
  const std::string fnName = TF_FUNC_NAME();
  {
    std::ostringstream logMsg;
    logMsg << "[tid=" << std::this_thread::get_id() << "] " << fnName
           << "\n  assetPath: " << assetPath
           << "\n  anchorAssetPath: " << anchorAssetPath.GetPathString();
    logger_->debug(logMsg.str());
  }

  auto result = catchAndLogExceptions(
      [&] {
        std::string identifier;

        if (manager_->isEntityReferenceString(assetPath)) {
          // If assetPath is an entity reference we must preserve it
          // unmodified as the "identifier", since it'll be passed to
          // subsequent member functions.  We assume it will (eventually)
          // resolve to an absolute path, making the anchorAssetPath redundant
          // (for now).
          // TODO(DF): Allow the manager to provide an identifier representing
          //  an "anchored" entity reference.
          identifier = assetPath;
        } else {
          identifier = ArDefaultResolver::_CreateIdentifier(
              assetPath,
              locationInManagerContextForEntity(manager_, readContext_, anchorAssetPath));
        }

        return identifier;
      },
      logger_, fnName);

  {
    std::ostringstream logMsg;
    logMsg << "[tid=" << std::this_thread::get_id() << "] "
           << "result: " << result;
    logger_->debug(logMsg.str());
  }
  return result;
}

// TODO(DF): Implement for publishing workflow.
std::string UsdOpenAssetIOResolver::_CreateIdentifierForNewAsset(
    const std::string &assetPath, const ArResolvedPath &anchorAssetPath) const {
  auto result = ArDefaultResolver::_CreateIdentifierForNewAsset(assetPath, anchorAssetPath);
  logger_->debug(TF_FUNC_NAME());

  logger_->debug("  assetPath: " + assetPath);
  logger_->debug("  anchorAssetPath: " + anchorAssetPath.GetPathString());
  logger_->debug("  result: " + result);

  return result;
}

ArResolvedPath UsdOpenAssetIOResolver::_Resolve(const std::string &assetPath) const {
  const std::string fnName = TF_FUNC_NAME();
  {
    std::ostringstream logMsg;
    logMsg << "[tid=" << std::this_thread::get_id() << "] " << fnName
           << "\n  assetPath: " << assetPath;
    logger_->debug(logMsg.str());
  }

  auto result = catchAndLogExceptions(
      [&] {
        if (manager_->isEntityReferenceString(assetPath)) {
          // TODO(DF): We may wish to do more here, e.g.
          //  `finalizedEntityVersion()`
          return ArResolvedPath{assetPath};
        }
        return ArDefaultResolver::_Resolve(assetPath);
      },
      logger_, fnName);

  {
    std::ostringstream logMsg;
    logMsg << "[tid=" << std::this_thread::get_id() << "] "
           << "result: " << result.GetPathString();
    logger_->debug(logMsg.str());
  }

  return result;
}

// TODO(DF): Implement for publishing workflow.
ArResolvedPath UsdOpenAssetIOResolver::_ResolveForNewAsset(const std::string &assetPath) const {
  auto result = ArDefaultResolver::_ResolveForNewAsset(assetPath);

  logger_->debug(TF_FUNC_NAME());
  logger_->debug("  assetPath: " + assetPath);
  logger_->debug("  result: " + result.GetPathString());

  return result;
}

/* Asset Operations*/
std::string UsdOpenAssetIOResolver::_GetExtension(const std::string &assetPath) const {
  const std::string fnName = TF_FUNC_NAME();
  {
    std::ostringstream logMsg;
    logMsg << "[tid=" << std::this_thread::get_id() << "] " << fnName
           << "\n  assetPath: " << assetPath;
    logger_->debug(logMsg.str());
  }

  auto result = catchAndLogExceptions(
      [&] {
        // TODO(DF): Give the manager a chance to provide the file extension.
        return ArDefaultResolver::_GetExtension(
            locationInManagerContextForEntity(manager_, readContext_, assetPath));
      },
      logger_, fnName);

  {
    std::ostringstream logMsg;
    logMsg << "[tid=" << std::this_thread::get_id() << "] "
           << "result: " << result;
    logger_->debug(logMsg.str());
  }

  return result;
}

ArAssetInfo UsdOpenAssetIOResolver::_GetAssetInfo(const std::string &assetPath,
                                                  const ArResolvedPath &resolvedPath) const {
  const std::string fnName = TF_FUNC_NAME();
  {
    std::ostringstream logMsg;
    logMsg << "[tid=" << std::this_thread::get_id() << "] " << fnName
           << "\n  assetPath: " << assetPath
           << "\n  resolvedPath: " << resolvedPath.GetPathString();
    logger_->debug(logMsg.str());
  }
  // TODO(DF): Determine if there is value in supporting this via the
  //  manager, including any useful data that can be stuffed into the
  //  generic `resolverInfo` VtValue member.
  auto result = ArDefaultResolver::_GetAssetInfo(assetPath, resolvedPath);

  {
    std::ostringstream logMsg;
    logMsg << "[tid=" << std::this_thread::get_id() << "] "
           << "\n  result.assetName: " << result.assetName
           << "\n  result.repoPath: " << result.repoPath;
    logger_->debug(logMsg.str());
  }

  return result;
}

ArTimestamp UsdOpenAssetIOResolver::_GetModificationTimestamp(
    const std::string &assetPath, const ArResolvedPath &resolvedPath) const {
  const std::string fnName = TF_FUNC_NAME();
  {
    std::ostringstream logMsg;
    logMsg << "[tid=" << std::this_thread::get_id() << "] " << fnName
           << "\n  assetPath: " << assetPath
           << "\n  resolvedPath: " << resolvedPath.GetPathString();
    logger_->debug(logMsg.str());
  }

  auto result = catchAndLogExceptions(
      [&] {
        if (manager_->isEntityReferenceString(assetPath)) {
          // Deliberately use a valid fixed timestamp, which will force caching
          // until we implement a more considered method.
          // TODO(DF): Consider how best to handle this via OpenAssetIO.
          return ArTimestamp{0};
        }
        return ArDefaultResolver::_GetModificationTimestamp(assetPath, resolvedPath);
      },
      logger_, fnName);

  {
    std::ostringstream logMsg;
    logMsg << "[tid=" << std::this_thread::get_id() << "] "
           << "result: " << result.GetTime();
    logger_->debug(logMsg.str());
  }

  return result;
}

std::shared_ptr<ArAsset> UsdOpenAssetIOResolver::_OpenAsset(
    const ArResolvedPath &resolvedPath) const {
  const std::string fnName = TF_FUNC_NAME();
  {
    std::ostringstream logMsg;
    logMsg << "[tid=" << std::this_thread::get_id() << "] " << fnName
           << "\n  resolvedPath: " << resolvedPath.GetPathString();
    logger_->debug(logMsg.str());
  }

  auto result = catchAndLogExceptions(
      [&] {
        return ArDefaultResolver::_OpenAsset(
            locationInManagerContextForEntity(manager_, readContext_, resolvedPath));
      },
      logger_, fnName);

  {
    std::ostringstream logMsg;
    logMsg << "[tid=" << std::this_thread::get_id() << "] "
           << "result: " << result.get();
    logger_->debug(logMsg.str());
  }
  return result;
}

// TODO(DF): Implement for publishing workflow.
bool UsdOpenAssetIOResolver::_CanWriteAssetToPath(const ArResolvedPath &resolvedPath,
                                                  std::string *whyNot) const {
  auto result = ArDefaultResolver::CanWriteAssetToPath(resolvedPath, whyNot);

  logger_->debug(TF_FUNC_NAME());
  logger_->debug("  resolvedPath: " + resolvedPath.GetPathString());
  logger_->debug("  result: " + std::to_string(static_cast<int>(result)));

  return result;
}

// TODO(DF): Implement for publishing workflow.
std::shared_ptr<ArWritableAsset> UsdOpenAssetIOResolver::_OpenAssetForWrite(
    const ArResolvedPath &resolvedPath, WriteMode writeMode) const {
  logger_->debug(TF_FUNC_NAME());
  logger_->debug("  resolvedPath: " + resolvedPath.GetPathString());

  return ArDefaultResolver::_OpenAssetForWrite(resolvedPath, writeMode);
}
