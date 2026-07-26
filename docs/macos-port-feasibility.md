# macOS Port Feasibility

## Executive summary

Porting Hesiod to native Apple-silicon macOS is feasible.

The recommended approach is to keep the existing C++20 and Qt 6 desktop
application, add macOS platform support to the shared build, and distribute a
signed and notarized `.app` in a DMG outside the Mac App Store.

This is a build, packaging, and compatibility project rather than a UI rewrite.
The existing Qt Widgets interface, Qt WebEngine heightmapper, terrain algorithms,
and OpenGL renderer can all be retained for an initial macOS release.

| Scope | Estimated effort for one experienced C++/Qt/macOS developer |
| --- | ---: |
| Build proof of concept on Apple silicon | about 1 person-week |
| Usable arm64 `.app` with bundled assets and dependencies | 3–4 person-weeks total |
| Signed, notarized, tested production release | 8–10 person-weeks total |

Confidence is high for the proof of concept and medium-high for a production
release. The estimates assume the first release keeps the existing OpenGL and
OpenCL paths rather than migrating them to Metal.

## Assessment method

The repository and all recursive submodules were inspected locally. Kimi Code
0.29.1 was run from the repository with the `kimi-code/k3` model and four
parallel audit agents. Its session can be resumed with:

```sh
kimi -r session_081fe8c1-2073-4b01-a0b3-038e47327d40
```

The audit covered the CMake configuration, Qt UI, renderer, compute code,
dependencies, packaging, tests, and licenses. A real configure and native build
attempt is being made on an arm64 Mac running macOS 27 and Xcode 27. The first
baseline configures successfully after installing GSL and the OpenCL C/C++
header packages. The `hesiod` arm64 executable now builds and its CLI smoke
test initializes the Apple M3 OpenCL GPU, compiles the terrain kernels, and
enables OpenMP. The macOS fork's draft pull request tracks the remaining app
bundle and packaging work.

The machine has arm64 builds of Qt, Qt WebEngine, OpenCV, assimp, Boost, Eigen,
GLEW, GLFW, GLM, GSL, libomp, nlohmann-json, spdlog, and the OpenCL headers.

## Why the port is viable

- The application is portable C++20 and Qt Widgets. Qt supports macOS and Apple
  silicon.
- QTerrainRenderer uses desktop OpenGL 3.3 Core. macOS exposes a compatible
  OpenGL 4.1 Core implementation on Apple silicon. OpenGL is deprecated, but it
  remains available.
- HighMap targets OpenCL 1.2. OpenCL is deprecated on macOS but remains available
  on Apple silicon.
- HighMap has CPU implementations for the important terrain operations and
  runtime GPU/CPU selection at the Hesiod node layer.
- The code contains no x86-only SIMD dependency. Apple Clang can compile the
  portable algorithms for arm64.
- Qt WebEngine supports macOS, so the embedded heightmapper does not need to be
  rewritten for a direct-download macOS release.

## Concrete blockers

### 1. Apple platforms are explicitly rejected

`CMakeLists.txt` aborts whenever `APPLE` is true, and
`cmake/HesiodPlatform.cmake` recognizes only Linux and Windows.

The port needs an `HSD_OS_MACOS` platform definition and Apple-specific build
branches rather than treating macOS as Linux.

### 2. Linux packaging is enabled by default

`HESIOD_ENABLE_GENERATE_APP_IMAGE` defaults to `ON`. It must default to `OFF` on
Apple so the Linux AppImage target is never included in a macOS build.

### 3. OpenMP discovery needs an Apple-Clang path

HighMap requires `OpenMP::OpenMP_CXX`. Homebrew `libomp` is already installed on
the test machine, but CMake may need explicit include and library hints for
Apple Clang.

Only a small part of the code uses OpenMP, so making it optional is a sensible
fallback and improves portability.

### 4. HighMap's OpenCL option is currently ineffective

`HIGHMAP_ENABLE_OPENCL` is declared but HighMap still unconditionally:

- calls `find_package(OpenCL REQUIRED)`;
- builds the OpenCL sources and CLWrapper;
- links `clwrapper` and `${OpenCL_LIBRARIES}`.

The shortest macOS path is to link Apple's OpenCL framework and retain the
existing GPU implementation. The durable fix is to make the option real and
support CPU-only builds as a fallback.

### 5. The renderer needs an explicit macOS context

QTerrainRenderer inherits `QOpenGLFunctions_3_3_Core` and its shaders use
`#version 330 core`. The application should request a 3.3 or 4.1 Core
`QSurfaceFormat` before creating its first widget. This avoids relying on Qt's
platform default.

No renderer rewrite is required for the first release. A Metal migration is a
future-proofing project, not a prerequisite.

### 6. The executable is not a macOS app bundle

The current target is a plain executable and copies `Hesiod/data` beside the
binary. A usable release needs:

- `MACOSX_BUNDLE`;
- bundle identifier, version, icon, and `Info.plist` metadata;
- assets under `Contents/Resources`;
- runtime asset lookup through `QCoreApplication`/`QStandardPaths`;
- `macdeployqt` packaging, including the Qt WebEngine helper process;
- code signing and notarization.

### 7. Build-system cleanup is needed

The audit also found:

- benchmark dependencies can be fetched during configuration unless HighMap
  benchmarks are disabled;
- some GCC/GNU-linker flags need Apple-Clang guards;
- Qt logging-rule variables appear to be misnamed;
- GLEW/GLUT include variables are referenced without corresponding package
  discovery;
- CI currently tests Ubuntu only.

These are bounded engineering tasks, not architectural blockers.

## Distribution and licensing

Hesiod and several submodules are GPLv3. A distributed macOS binary therefore
needs normal GPL compliance, including corresponding source availability.
HighMap's third-party license inventory also marks one adapted shader as
Creative Commons BY-NC-SA; its exact use and redistribution obligations should
be reviewed before any commercial release.

The first release should be a signed and notarized download from GitHub or
another website, not a Mac App Store submission:

- Qt documents Qt WebEngine as incompatible with the Mac App Store sandbox and
  review requirements.
- App Store terms add licensing questions for GPLv3 software.

This is not legal advice; commercial distribution should receive a focused
license review.

## Recommended phases

### Phase 0: build proof of concept — about 1 week

1. Add the macOS CMake platform branch and disable AppImage generation.
2. Configure Homebrew dependency discovery, including GSL and libomp.
3. Link Apple's OpenGL and OpenCL frameworks.
4. Request an explicit OpenGL Core profile.
5. Build and launch on Apple silicon.
6. Load an example graph, run CPU and GPU nodes, and exercise the 2D/3D viewers.

This phase answers the largest unknowns cheaply: dependency discovery, OpenCL
runtime behavior, and QTerrainRenderer compatibility.

### Phase 1: usable `.app` — 2–3 additional weeks

1. Create a proper app bundle and move assets into Resources.
2. Make all settings, temporary files, exports, and downloads use macOS-safe
   paths.
3. Package Qt, Qt WebEngine, and non-system libraries.
4. Add an arm64 macOS CI job and smoke tests.
5. Test file dialogs, external URLs, WebEngine downloads, batch mode, and common
   node graphs.

### Phase 2: production release — 4–6 additional weeks

1. Sign the app and helper processes with the hardened runtime.
2. Notarize and staple a DMG.
3. Add regression tests and golden terrain outputs.
4. Profile large graphs for memory use and UI stalls.
5. Improve long-running compute behavior and cancellation.
6. Document GPL source distribution and third-party notices.

## Recommended architecture

Keep one cross-platform repository. Add macOS conditionals to the existing build
and contribute generally useful fixes upstream where possible.

Do not create a long-lived source fork that rewrites the Qt UI or renderer.
Metal can be evaluated later if Apple removes legacy OpenGL/OpenCL support or if
performance data justifies the investment.

## Decision

Proceed with the macOS proof of concept.

The risk/reward is favorable: the first runnable build is likely a one-week
spike, and no major subsystem needs to be replaced. The main caveat is that the
production distribution target should be a notarized direct download rather
than the Mac App Store.
