# Third-party notices

## AviUtl ExEdit2 Plugin SDK

`third_party/aviutl2-sdk/plugin2.h` is from the AviUtl ExEdit2 Plugin SDK, provided by Kenkun under the MIT License. The associated copyright notice and license are included in [third_party/aviutl2-sdk/LICENSE.txt](third_party/aviutl2-sdk/LICENSE.txt). The header from the official SDK dated 2026-11-07 was converted from CP932 to UTF-8 for the build; its decoded text was otherwise verified unchanged. Other SDK files and AviUtl2 binaries are not included.

SDK distribution: https://spring-fragrance.mints.ne.jp/aviutl/  
License reference (unofficial SDK mirror): https://github.com/oov/aviutl2_plugin_sdk

The plugin uses the official header rather than handwritten interface declarations. A comparison of the fields used by the former declarations against this SDK passed. The SDK date alone does not establish incompatibility; runtime behavior with AviUtl2 2.1.13 has **not** yet been verified. See [SDK policy](docs/SDK_POLICY.md).

## PSDToolKit

The plugin optionally reads objects created by PSDToolKit. It does not bundle PSDToolKit or its source code. Obtain it from its own distributor and review its terms separately: https://github.com/oov/aviutl2_psdtoolkit2

## Microsoft tools and runtime

The build uses Microsoft Visual C++ and Windows SDK tools/libraries. Neither the compiler nor a redistributable package is included in this source distribution. Binary distribution, including any linked or separately bundled runtime components, must follow the applicable Microsoft license terms for the toolchain used to build it. See https://learn.microsoft.com/cpp/windows/redistributing-visual-cpp-files
