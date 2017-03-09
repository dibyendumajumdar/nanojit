# nanojit
Nanojit is a small, cross-platform C++ library that emits machine code. It is part of [Adobe ActionScript](https://github.com/adobe/avmplus) 
and [Mozilla SpiderMonkey](https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/Internals/Tracing_JIT).

## Standalone Build
The goal of this project is to create a standalone build of Nanojit. The original folder structure of avmplus is maintained so that merging 
upstream changes is easier. 

The new build is work in progress. A very early version of CMakeLists.txt is available, this has been tested only on Windows 10 with Visual Studio 2017.
The aim is to support the build on X86_64 processors, and Windows, Linux and Mac OSX.  

To create Visual Studio project files do following:

```
mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=Debug ..
```

Building the project will result in a standalone nanojit library and the executable lirasm which can be used to assemble and run standalone
code snippets. 

## Documentation
A secondary goal of this project is to create some documentation of the standalone library, and document how it can be used. 

## Why nanojit?
It seems that this is perhaps the only small JIT library that is available. 