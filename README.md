# Hornet

[![Build Status](https://secure.travis-ci.org/penberg/hornet.png?branch=master)](http://travis-ci.org/penberg/hornet)

Hornet is a Java virtual machine designed for low latency, high performance
applications.

## Features

* Interpreter
* Uses OpenJDK for standard class libraries
* Written in C++11
* Runs on Linux and Darwin

### Planned

* Pauseless GC
* LLVM ahead-of-compilation
* RTJS class library support

## Installation

First, install dependencies:

**Fedora**

```
$ yum install clang java-1.7.0-openjdk-devel
```

Then install Hornet:

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
