from conans import ConanFile, CMake

class UsdOpenAssetIOResolverConan(ConanFile):
    name = "UsdOpenAssetIOResolver"
    description = "UsdOpenAssetIOResolver"
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps"

    requires = ["openassetio/1.0.0-alpha.9"]

    def imports(self):
        self.copy(
            src="lib/python3.9",
            pattern="*",
            dst="lib/python3.9",
            root_package="openassetio",
        )
