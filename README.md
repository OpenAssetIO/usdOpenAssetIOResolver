# usdOpenAssetIOResolver

A USD AR2 plugin that hosts OpenAssetIO.

> **Note:**
> This project is in a proof-of-concept phase, to explore compatibility
> between the two ecosystems. It is in no way feature complete or
> optimized for performance.

Provides the capability to resolve entity references through any
OpenAssetIO enabled manager without the need for USD specific
programming.

Special attention has been made to ensure that this plugin can be used
in existing USD documents, preserving the `ArDefaultResolver`
search-path based resolution behaviour for path based references.

## Example

```usd
def "ParkingLot"
{
    def "ParkingLot_Floor_1" (
        references = @bal:///floor@</ParkingLot_Floor>
    )
    {
    }
}
```

An asset management system's OpenAssetIO entity reference can be used
anywhere the `@<ref>@` syntax is supported. In this case, a reference to
an asset managed by our test manager, the [Basic Asset
Library](https://github.com/OpenAssetIO/OpenAssetIO-Manager-BAL) is used.

## How entities are resolved

When USD creates an instance of the resolver, The default OpenAssetIO
manager will be initialized - see the
[docs](https://openassetio.github.io/OpenAssetIO/glossary.html#default_config_var)
for more on this mechanism.

The plugin then resolves the [`LocatableContentTrait`](https://github.com/OpenAssetIO/OpenAssetIO-MediaCreation/blob/659e641ce7e4dba6c5c6508c30b484fa31c1d61a/traits.yml#L54)
for each string that the configured manager claims as a reference, and
passes this path on for loading.

> **Note:**
> The `LocatableContentTrait` dictates that the resolved `location`
> should be an absolute path, and so the default USD search path
> mechanism will have no effect on the resolved result.

Asset variation based on the parent asset (`anchorAssetPath`) has not
yet been implemented, but will be added at a later date.

## Project status

This resolver is currently in prototype stage, it has several key
limitations.

- Python is currently required whilst the remaining work porting the
  core OpenAssetIO API to C++ is completed.

- `managementPolicy` is not checked, and so the configured manager will
  always be queried for each reference during stage composition.

- Only the `LocatableContentTrait` is resolved. File extensions and file
  modification times will are based on file-based fallbacks if possible,
  and return sensible defaults if not. Many potentially expected
  structures (ie, `AssetInfo`), will be empty.

- There is no USD locale set in the OpenAssetIO context.

- Only read workflows are supported. Methods related to writing data
  fall back to the `ArDefaultResolver`.

- Error handling is as yet quite primitive, an incomplete configuration
  may provide cryptic and hard to understand errors. You are advised to
  be extra sure all your environment variables are setup correctly.

- The implementation has in not been optimized for performance.

## Dependencies

### [OpenAssetIO](https://github.com/OpenAssetIO/OpenAssetIO/) (1.0.0-alpha.9)

Currently, OpenAssetIO must be built from source to build
`usdOpenAssetIOResolver`.

The steps to do this can be found
[here.](https://github.com/OpenAssetIO/OpenAssetIO/blob/main/doc/BUILDING.md)

### [USD](https://github.com/PixarAnimationStudios/USD) (22.11)

Install USD by whatever means, the most straightforward being to check
out the repo and use the provided
[build_usd.py](https://github.com/PixarAnimationStudios/USD/tree/release/build_scripts)
script. If you elect to install USD to a custom location, remember to
take a note of it, so you can pass it to `CMAKE_PREFIX_PATH` below.

The specific versions listed for these dependencies are simply the
versions that the resolver was tested against during development, it's
possible other versions will function.

## Building

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/openassetio/dist/;/path/to/USD
cmake --build build
cmake --install build
```

> **Note**
>
> VFX Reference Platform CY2022 requires the old pre c++11 ABI, which is
> used by default for `gcc` builds of these projects. It is important
> that `usdOpenAssetIOResolver`, `OpenAssetIO` and `USD` are all built
> under the same ABI.
>
> You can build under the new ABI by appending
> `-DOPENASSETIO_USDRESOLVER_GLIBCXX_USE_CXX11_ABI=On` to your configure
> command.

## Running

To enable the resolver, before running any USD application, you must set
the `PXR_PLUGINPATH_NAME` environment variable to point to
`plugInfo.json`.

You also need to configure `OpenAssetIO` with a manager. Use the
[OPENASSETIO_DEFAULT_CONFIG](https://openassetio.github.io/OpenAssetIO/glossary.html#default_config_var)
variable.

```sh
export PXR_PLUGINPATH_NAME=$(pwd)/build/dist/resources/plugInfo.json
export OPENASSETIO_DEFAULT_CONFIG=path/to/config/openassetio.conf
usdcat yourUsdFile.usd
```

> **Note**
>
> Depending on you install mechanism, you may also need to extend your
> system paths (eg `LD_LIBRARY_PATH` and `PYTHONPATH`) to reference
>`OpenAssetIO` and `Usd`.
>
> For an example of this, and a whole build/run configuration, see
> [test.yml](.github/workflows/test.yml), which utilizes the
> [openassetio-build](https://github.com/openassetio/OpenAssetIO/pkgs/container/openassetio-build)
> and [ci-vfxall](https://hub.docker.com/r/aswf/ci-vfxall) Docker
> containers to manage dependencies and install/run the resolver.

## Debug logging

Before running any USD application

```sh
export TF_DEBUG=OPENASSETIO_RESOLVER
export OPENASSETIO_LOGGING_SEVERITY=1
```

To enable debug logging from the resolver.

## Testing

To run tests, from the project root

```sh
export PXR_PLUGINPATH_NAME=$(pwd)/build/dist/resources/plugInfo.json
cd tests
python -m pip install -r requirements.txt
pytest
```

> **Note**
>
> Refer to the [Running](#running) section for the environmental
> prerequisites to run these tests.

## A note on Python

One of the goals of OpenAssetIO is to provide a flexible API surface for
a variety of production use cases. We have seen many situations where
asset management code is only present in interpreted languages such as
Python.

Python however, has a huge performance impact due to its use of the GIL.

There has been significant interest in the ability to use Python-based
code within the USD context. OpenAssetIO allows managers to write their
code in either Python, or C/C++ depending on the performance
characteristics required.

This plugin facilitates code in either language to be used within USD.

We _fully understand the concerns this may bring_, especially in the
content of real-world production at scale, and plan to tackle this on
several fronts:

- _OpenAssetIO operates without Python in the case that C/C++
  implementations of required methods are provided._
- OpenAssetIO will provide an (optional) out-of-process Python
  interpreter to avoid UI thread locking when the host tool uses python
  for its user interface.
- OpenAssetIO allows for mixed implementations, such that high call
  volume methods such as `resolve` can be written in C++, and low volume
  methods (eg: `register`) can be implemented in Python.
