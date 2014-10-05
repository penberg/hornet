# Hornet

[![Build Status](https://secure.travis-ci.org/penberg/hornet.png?branch=master)](http://travis-ci.org/penberg/hornet)

Hornet is an implementation of the Java virtual machine with emphasis on
predictable execution for applications that have low-latency requirements.
Planned features include pauseless garbage collection, ahead-of-time
compilation, off-heap memory management, FFI, object layout control, and
observability APIs for analyzing and optimizing low-latency applications.

## Features

_Hornet is in pre-alpha state so the features are still very much
work-in-progress._

* [MPS](http://www.ravenbrook.com/project/mps/) based incremental GC with very
  low pause times.
* Execution engines:
    * Fast interpreter
    * Baseline DynASM-based compiler
    * Optimizing LLVM-based compiler
* OpenJDK classlib
* Supports Linux and Darwin
* Written in C++11

## Design

* [Launcher](hornet.cc) is the ``java``-like front-end.
* [Translator](java/translator.cc) is a bytecode translator that is shared by
  the fast interpreter and both compilers.
* [Fast interpreter](java/interp.cc) is a portable interpreter that
  translates bytecode into an internal format that is faster to interpret.
* [Baseline compiler](java/dynasm.cc) is based on Dynasm framework that is used
  by LuaJIT, for example.
* [Optimizing compiler](java/llvm.cc) is based on LLVM.
* [GC](mps/mps.c) is based on [MPS](http://www.ravenbrook.com/project/mps/).

## Installation

First, install dependencies:

**Fedora**

```
$ yum install clang java-1.7.0-openjdk-devel
```

If you want to enable the [DynASM](http://luajit.org/dynasm.html) backend,
install LuaJIT:

```
$ yum install luajit
```

If you want to enable the [LLVM](http://llvm.org/) backend, install the
library:

```
$ yum install llvm-dev
```

Finally, install Hornet:

```
$ make install
```

The command installs an executable ``hornet`` to ``$HOME/bin``.

## Usage

Hornet works like ``java``:

```
$ hornet
usage: hornet [-options] class [args...]
```

## License

Copyright © 2013 Pekka Enberg and contributors

Hornet is distributed under the 2-clause BSD license.
