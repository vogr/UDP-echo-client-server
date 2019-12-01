MAKEFLAGS += --warn-undefined-variables --no-builtin-rules


NAMES := server fancy-client
SRC_FILES := server.c fancy-client.c
# Choose between BUILD
BUILD := release

ARCHIVE := UDP_OGIER.tar.gz
ARCHIVE_FILES := server.c server.h fancy-client.c Makefile

ifeq ($(BUILD),debug)
SUFFIX := .debug
else ifeq ($(BUILD),release)
SUFFIX :=
else
$(error Unknown BUILD type $(BUILD). Accepted values are BUILD=debug and BUILD=release)
endif

ifndef ($(ARGS))
ARGS :=
endif

OBJDIR := objects
OBJECTS := $(SRC_FILES:%.c=$(OBJDIR)/%.o)
# Auto dependency genereation for gcc (see http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/)
DEPDIR := $(OBJDIR)/deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d


# see https://src.fedoraproject.org/rpms/redhat-rpm-config/blob/master/f/buildflags.md
# and https://nullprogram.com/blog/2019/11/15/ (for -z noexecstack)
CFLAGS.debug := -ggdb3 -O0 # -Og # Og currently optimizes out too many variables (see https://gcc.gnu.org/bugzilla//show_bug.cgi?id=78685)
CFLAGS.release := -O2 -flto
CFLAGS ?= -std=gnu11 \
	-Wall -Wextra -Wpedantic \
	-Wformat=2 \
	-Wswitch-default -Wswitch-enum \
	-Wfloat-equal \
	-Wconversion \
	-pipe \
	-D_FORTIFY_SOURCE=2 \
	-fexceptions \
	-fasynchronous-unwind-tables \
	-fstack-protector-strong \
	-fstack-clash-protection \
	-fcf-protection \
	-Wl,-z,noexecstack \
	$(CFLAGS.$(BUILD))
# Note : `-fstack-protector-strong`, `-fstack-clash-protection`, `-fcf-protection` add runtime
# checks and may therefore have a performance impact.


.PHONY: all
all: $(NAMES)

.PHONY: redo
redo: clean
	$(MAKE) all

server: objects/server.o
	$(CC) $^ -o $@ $(CFLAGS)

fancy-client: objects/fancy-client.o
	$(CC) $^ -o $@ $(CFLAGS)

$(OBJECTS): $(OBJDIR)/%.o: %.c
	@mkdir -p $(OBJDIR) $(DEPDIR)
	$(CC) -c $< -o $@ $(CFLAGS) $(DEPFLAGS)

archive: $(ARCHIVE_FILES)
	tar caf $(ARCHIVE) $^

.PHONY: clean
clean:
	rm -f $(NAMES) $(ARCHIVE)
	if [ -d objects ]; then rm -r objects; fi

DEPFILES := $(SRC_FILES:%.c=$(DEPDIR)/%.d)
include $(wildcard $(DEPFILES))
