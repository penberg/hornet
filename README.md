# Hornet

[![Build Status](https://secure.travis-ci.org/penberg/hornet.png?branch=master)](http://travis-ci.org/penberg/hornet)

Hornet is a Java virtual machine designed for low latency, high performance
applications.

## Features

* [MPS](http://www.ravenbrook.com/project/mps/) based incremental GC with very low pause times.
* Multiple backends:
    * Interpreter
    * DynASM (x86-64)
    * LLVM
* Uses OpenJDK for standard class libraries
* Written in C++11
* Runs on Linux and Darwin

### Planned

* LLVM ahead-of-compilation
* RTJS class library support

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
