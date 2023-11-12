# wasm-ecs-game-profile
Demo - excerpt of a bullet-heaven game with profiling to compare WebAssembly and native builds of a game.

## Building and Running (native binary)

Native builds are pretty standard CMake fare:

```bash
mkdir out && cd out
cmake ..
make igdemo
./igdemo
```

## Building and Running (WASM binary)

WebAssembly builds are a bit more involved.

First, perform a native build of tooling. This is only required once, and you can use
the same output directory as your native builds, if you already ran those:

```bash
mkdir out/native && cd out/native
cmake ../..
make flatc igpack-gen
```

Also, before running any WASM build, make sure the Emscripten environment is set up:

```bash
emsdk activate latest
emsdk_env
```

With tools built, WASM builds can be run.

```bash
mkdir out/wasm && cd out/wasm
emcmake cmake -DIG_TOOL_WRANGLE_PATH="../native/wrangle-tools.cmake" ../..
emmake make igdemo
```

The `IG_TOOL_WRANGLE_PATH` property can be overridden in the native build to put the
tool wrangle CMake file somewhere else, just make sure that the generated file is
properly loaded by the WASM build.

Running the WASM build also requires a bit of love, I have provided a simple web
server that attaches all the required headers and correctly hunts for source maps
on debug builds:

```bash
# From out/wasm
node ../../webview/game-server.js --port=8000
```

The `--port` argument is optional, and defaults to 8000.

Open a browser to `http://localhost:8000/index.html` to view the demo.

## Folder structure:

* assets: Artistic assets used to build and run the project
	* NOTICE: Not all raw assets used in online demos are included! I don't have all the redistribution rights for everything in the online demo.
	* I'm working to fix this, but I wanted to publish my findings before I had assets ready.
* cmake: CMake script files with helper functions
* extern: CMake script to load 3rd party dependencies + clones of 3rd party libraries used (where needed)
	* digraph: Third-party library for creation, traversing, and processing of digraphs
* igdemo: C++ source code for the game demo itself
* include: Header files for the game demo itself
* lib: Half-baked libraries that aren't mature enough (yet) to separate into their own projects.
	* These are directly copy-pasted from another one-off game project
	* igasset: Library for unpacking compressed+packaged files generated by igpack-gen tool
	* igecs: ECS library built on EnTT and igasync, with execution graph and profiling support
* tools: Source for tools used by the project
	* These tools are built on native builds, and must be provided to WASM builds - they cannot be built as WASM modules!
	* enumerate_assimp: Not actually needed, but a helpful little utility to list what resources are in a file that can be loaded by Assimp
	* igpack-gen: Tool to compress and package resource files (geometry, shader source, textures) into *.igpack files that can be loaded by igasset library
* webview: Assorted HTML and JavaScript sources for running the WASM builds locally

## External projects used:

We all stand on the shoulders of giants, this project is no exception.

Thank you so much for the work of others in the community who build and maintain these fantastic C++ libraries - this project would not have been possible without the following third-party dependencies:

* [Digraph](https://github.com/grame-cncm/digraph) - simple C++11 directed graph library
* [assimp](https://github.com/assimp/assimp) - Importer library for 3D geometry, armatures, skeletal animations, textures, and scene graphs. Used in asset packaging pipeline, but not in game itself.
* [draco](https://github.com/google/draco) - 3D geometry format optimized for web use cases (highly compressed 3D geometry)
* [flatbuffers](https://github.com/google/flatbuffers) - Binary serialization format, similar to protocol buffers but a bit more optimized for gaming use cases.
* [EnTT](https://github.com/skypjack/entt) - Entity Component System (ECS) library
* [CLI11](https://github.com/CLIUtils/CLI11) - CLI argument handling library, useful especially for tooling
* [Ozz Animation](https://github.com/guillaumeblanc/ozz-animation) - Excellent WASM-friendly 3D skeletal animation library
* [nlohmann/json](https://github.com/nlohmann/json) - JSON library for C++ with a wonderfully developer-friendly API

I also use a couple of libraries that I've authored for independent use.
They're mature enough to publish, not quite mature enough to be considered production-ready,
but they've done well enough for my hobby projects:

* [igasync](https://github.com/sessamekesh/igasync) - C++ concurrency library, friendly to browser threading constraints
* [iggpu](https://github.com/sessamekesh/iggpu) - Simple wrapper around Google Dawn WebGPU implementation library and WebGPU utilities for C++