JAVA_HOME = /usr/lib/jvm/java

PREFIX ?= $(HOME)

DESTDIR=
BINDIR=$(PREFIX)/bin

CXX = clang++

ifneq ($(WERROR),0)
	CXXFLAGS_WERROR = -Werror
endif

WARNINGS = -Wall -Wextra $(CXXFLAGS_WERROR) -Wno-unused-parameter
INCLUDES = -Iinclude -I$(JAVA_HOME)/include/ -I$(JAVA_HOME)/include/linux
OPTIMIZATIONS = -O3
CXXFLAGS = $(OPTIMIZATIONS) $(WARNINGS) $(INCLUDES) -g -std=c++11 -MMD

PROGRAMS = hornet

INST_PROGRAMS += hornet

OBJS += java/class_file.o
OBJS += java/constant_pool.o
OBJS += java/interp.o
OBJS += java/jar.o
OBJS += java/jni.o
OBJS += java/loader.o
OBJS += java/verify.o
OBJS += vm/alloc.o
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
	$(Q) $(CXX) $(CXXFLAGS) $(OBJS) -lzip hornet.cc -o hornet

define INSTALL_EXEC
	install -v $1 $(DESTDIR)$2/$1 || exit 1;
endef

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
	$(Q) rm -f $(PROGRAMS) $(OBJS) $(DEPS) hornet.d

tags TAGS:
	rm -f -- "$@"
	find . -name "*.cc" -o -name "*.hh" -o -name "*.h" -o -name "*.c" |\
		xargs $(if $(filter $@, tags),ctags,etags) -a
.PHONY: tags TAGS

-include $(DEPS)
