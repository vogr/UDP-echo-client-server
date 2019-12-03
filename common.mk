
# This file contains rules and variables shared between the Makefiles in the
# project subdirectories.


# Choose between BUILD=release and BUILD=debug
BUILD := release

ifeq ($(BUILD),debug)
SUFFIX := .debug
else ifeq ($(BUILD),release)
SUFFIX :=
else
$(error Unknown BUILD type $(BUILD). Accepted values are BUILD=debug and BUILD=release)
endif

ARGS :=

BIN := $(NAME)$(SUFFIX)
OBJDIR := objects$(SUFFIX)
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
all: $(BIN)

.PHONY: run
run: $(BIN)
	./$(BIN) $(ARGS)

.PHONY: redo
redo: clean
	$(MAKE) all

.PHONY: rerun
rerun: redo
	$(MAKE) run ARGS=$(ARGS)

.PHONY: record
record: $(BIN)
	rr record ./$(BIN) $(ARGS)

.PHONY: gdbgui
gdbgui:
	$(MAKE) record ARGS=$(ARGS)
	gdbgui --rr

$(BIN): $(OBJECTS)
	$(CC) $^ -o $@ $(CFLAGS)

$(OBJECTS): $(OBJDIR)/%.o: %.c
	@mkdir -p $(OBJDIR) $(DEPDIR)
	$(CC) -c $< -o $@ $(CFLAGS) $(DEPFLAGS)


.PHONY: clean
clean:
	rm -f $(NAME) $(NAME).debug
	if [ -d objects ]; then rm -r objects; fi
	if [ -d objects.debug ]; then rm -r objects.debug; fi

DEPFILES := $(SRC_FILES:%.c=$(DEPDIR)/%.d)
include $(wildcard $(DEPFILES))
