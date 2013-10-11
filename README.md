# Hornet

Hornet is a Java virtual machine written in C++11.  It uses OpenJDK for
runtime services, and builds and runs on x86-64 Linux machines.

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

Copyright © 2013 Pekka Enberg

Hornet is distributed under the 3-clause BSD license.
