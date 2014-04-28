uname_M := $(shell sh -c 'uname -m 2> /dev/null || echo not')
uname_S := $(shell sh -c 'uname -s 2> /dev/null || echo not')

JAVA_HOME ?= /usr/lib/jvm/java

PREFIX ?= $(HOME)

DESTDIR=
BINDIR=$(PREFIX)/bin

CXX ?= clang++

LLVM_CONFIG ?= llvm-config

LLVM_VERSION = $(shell $(LLVM_CONFIG) --version)

LUAJIT ?= luajit

LUAJIT_VERSION = $(shell $(LUAJIT) -v 2>/dev/null)

ifneq ($(WERROR),0)
	CXXFLAGS_WERROR = -Werror
endif

WARNINGS = -Wall -Wextra $(CXXFLAGS_WERROR) -Wno-unused-parameter
INCLUDES = -Iinclude -I$(JAVA_HOME)/include/ $(LIBZIP_INCLUDES)
OPTIMIZATIONS = -O3
CXXFLAGS = $(OPTIMIZATIONS) $(CONFIGURATIONS) $(WARNINGS) $(INCLUDES) -g -std=c++11 -MMD

#
# LLVM
#

ifeq ($(LLVM_VERSION),3.4)
	LLVM_LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags)
	LLVM_LIBS = $(shell $(LLVM_CONFIG) --libs)

	OBJS += java/llvm.o

	CONFIGURATIONS += -DCONFIG_HAVE_LLVM
	CXXFLAGS += -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS
	LDFLAGS += $(LLVM_LDFLAGS)
	LIBS += $(LLVM_LIBS)
else
$(warning LLVM not found, disables LLVM support. Please install llvm-devel or llvm-dev)
endif

#
# DynASM
#

ifneq ($(LUAJIT_VERSION),)
ifeq ($(uname_M),x86_64)
	INCLUDES += -Idynasm
	CONFIGURATIONS += -DCONFIG_HAVE_DYNASM
	OBJS += java/dynasm.o
else
$(warning DynaASM is only supported on x86-64, disabling it.)
endif
else
$(warning LuaJIT not found, disables DynaASM support. Please install luajit)
endif

ifeq ($(uname_S),Darwin)
	INCLUDES += -I$(JAVA_HOME)/include/darwin
	CONFIGURATIONS += -DCONFIG_NEED_MADV_HUGEPAGE
	CONFIGURATIONS += -DCONFIG_NEED_MAP_ANONYMOUS
else
	INCLUDES += -I$(JAVA_HOME)/include/linux
endif

PROGRAMS = hornet

INST_PROGRAMS += hornet

OBJS += java/backend.o
OBJS += java/class_file.o
OBJS += java/constant_pool.o
OBJS += java/interp.o
OBJS += java/jar.o
OBJS += java/jni.o
OBJS += java/loader.o
OBJS += java/translator.o
OBJS += java/verify.o
OBJS += java/zip.o
OBJS += vm/alloc.o
OBJS += vm/field.o
OBJS += vm/gc.o
OBJS += vm/jvm.o
OBJS += vm/klass.o
OBJS += vm/method.o
OBJS += vm/thread.o

DEPS = $(OBJS:.o=.d)

# Make the build silent by default
V =
ifeq ($(strip $(V)),)
	E = @echo
	Q = @
else
	E = @\#
	Q =
endif
export E Q

all: $(PROGRAMS)

hornet: $(OBJS) hornet.cc
	$(E) "  LINK  " $@
	$(Q) $(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) $(LIBS) -lz hornet.cc -o hornet

define INSTALL_EXEC
	install -v $1 $(DESTDIR)$2/$1 || exit 1;
endef

java/dynasm.cc: java/dynasm_x64.h

%.h: %.dasc
	$(E) "  DASM  " $@
	$(Q) luajit dynasm/dynasm.lua $< > $@ 

%.o: %.cc
	$(E) "  CXX   " $@
	$(Q) $(CXX) -c $(CXXFLAGS) $< -o $@


install: all
	$(E) "  INSTALL "
	$(Q) install -d $(DESTDIR)$(BINDIR)
	$(Q) $(foreach f,$(INST_PROGRAMS),$(call INSTALL_EXEC,$f,$(BINDIR)))
.PHONY: install

check: $(PROGRAMS)
	$(E) "  CHECK   "
	$(Q) $(shell scripts/test)
.PHONY: check

clean:
	$(E) "  CLEAN"
	$(Q) rm -f $(PROGRAMS) $(OBJS) $(DEPS) hornet.d java/dynasm_x64.h

tags TAGS:
	rm -f -- "$@"
	find . -name "*.cc" -o -name "*.hh" -o -name "*.h" -o -name "*.c" |\
		xargs $(if $(filter $@, tags),ctags,etags) -a
.PHONY: tags TAGS

-include $(DEPS)
