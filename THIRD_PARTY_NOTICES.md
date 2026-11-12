# Third-party notices

## AviUtl ExEdit2 Plugin SDK

`third_party/aviutl2-sdk/plugin2.h` and `module2.h` contain declarations from the AviUtl ExEdit2 Plugin SDK, provided by Kenkun under the MIT License. The associated copyright notice and license are included in [third_party/aviutl2-sdk/LICENSE.txt](third_party/aviutl2-sdk/LICENSE.txt). `plugin2.h` was converted from CP932 to UTF-8 and its decoded text was otherwise verified unchanged. `module2.h` contains the official declarations required by `SRTrack.mod2`, normalized for the local build. Other SDK files and AviUtl2 binaries are not included.

SDK distribution: https://spring-fragrance.mints.ne.jp/aviutl/  
License reference (unofficial SDK mirror): https://github.com/oov/aviutl2_plugin_sdk

The plugin and Preview module use the official headers rather than handwritten interface declarations. The SDK date alone does not establish incompatibility. Standard-text subtitle extraction, SRT output, and basic Preview-module operation have been verified with AviUtl2 2.1.13; compatibility of the full feature set remains unverified. See [SDK policy](docs/SDK_POLICY.md).


## Microsoft tools and runtime

The build uses Microsoft Visual C++ and Windows SDK tools/libraries. Neither the compiler nor a redistributable package is included in this source distribution. Binary distribution, including any linked or separately bundled runtime components, must follow the applicable Microsoft license terms for the toolchain used to build it. See https://learn.microsoft.com/cpp/windows/redistributing-visual-cpp-files
