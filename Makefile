# =============================================================================
# Makefile for jala
# Persian (Jalali) calendar in the terminal
# =============================================================================

# =============================================================================
# Configuration
# =============================================================================

TARGET      := jala
SRCDIR      := src
SRCS        := $(SRCDIR)/main.cpp \
               $(SRCDIR)/jalali.cpp \
               $(SRCDIR)/gregorian.cpp \
               $(SRCDIR)/holidays.cpp
HDRS        := $(SRCDIR)/jalali.hpp \
               $(SRCDIR)/gregorian.hpp \
               $(SRCDIR)/holidays.hpp \
               $(SRCDIR)/holidays_data.hpp
OBJS        := $(SRCS:.cpp=.o)

TEST_SRC    := tests/jalali_test.cpp
TEST_BIN    := tests/jalali_test

MANPAGE     := jala.1
PREFIX      ?= /usr/local
BINDIR      := $(PREFIX)/bin
MANDIR      := $(PREFIX)/share/man/man1

CXX         ?= g++
CXXFLAGS    ?= -std=c++17 -O2 -g -Wall -Wextra -I$(SRCDIR)
LDFLAGS     ?=
LDLIBS      ?=

# -----------------------------------------------------------------------------
# Generated files
# -----------------------------------------------------------------------------
# These are produced by the Python scripts under scripts/ and are not
# tracked by git. The Makefile regenerates them whenever the JSON
# source or the generator itself changes.

GEN_HPP        := $(SRCDIR)/holidays_data.hpp
GEN_OVERRIDES  := data/overrides.json

JSON_SRC       := data/holidays.json
JSON_MANUAL    := data/manual_overrides.json
PY_NEUTRALIZE  := scripts/neutralize_overrides.py
PY_EMBED       := scripts/embed_holidays.py

# =============================================================================
# Targets
# =============================================================================

.PHONY: all clean install uninstall test test-unit debug holidays

all: $(TARGET)

# -----------------------------------------------------------------------------
# Generated data
# -----------------------------------------------------------------------------

# Regenerate every derived data file by hand (useful after editing
# the JSON source or the rule tables). Most users do not need to run
# this; the build rules below invoke the scripts automatically.
holidays:
	python3 $(PY_NEUTRALIZE)
	python3 $(PY_EMBED)

# data/overrides.json is derived from the upstream holiday list and
# the hand-edited manual overrides.
$(GEN_OVERRIDES): $(JSON_SRC) $(JSON_MANUAL) $(PY_NEUTRALIZE)
	python3 $(PY_NEUTRALIZE)

# src/holidays_data.hpp is derived from the upstream holiday list and
# the overrides produced above.
$(GEN_HPP): $(JSON_SRC) $(GEN_OVERRIDES) $(PY_EMBED)
	python3 $(PY_EMBED)

# -----------------------------------------------------------------------------
# Main binary
# -----------------------------------------------------------------------------

# Link the executable from all object files
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

# Compile each source file to an object file. The headers are listed
# as dependencies so any change triggers a full rebuild.
$(SRCDIR)/%.o: $(SRCDIR)/%.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Build with debug symbols and without optimization
debug: CXXFLAGS += -DDEBUG -O0
debug: clean $(TARGET)

# -----------------------------------------------------------------------------
# Installation
# -----------------------------------------------------------------------------

install: $(TARGET) $(MANPAGE)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	install -d $(DESTDIR)$(MANDIR)
	install -m 644 $(MANPAGE) $(DESTDIR)$(MANDIR)/$(MANPAGE)
	@echo "Installed $(TARGET) to $(DESTDIR)$(BINDIR)"
	@echo "Installed $(MANPAGE) to $(DESTDIR)$(MANDIR)"

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(MANDIR)/$(MANPAGE)
	@echo "Uninstalled $(TARGET)"

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------

# Run the shell-based integration tests (require the main binary)
test: $(TARGET)
	@echo "Running integration tests..."
	@./tests/run_tests.sh

# Build the unit-test binary and run it
test-unit: $(TEST_BIN)
	@echo "Running unit tests..."
	@$(TEST_BIN)

# The unit tests link against jalali.cpp, gregorian.cpp, and
# holidays.cpp directly, with no dependency on main.cpp or the CLI.
# They run in a few milliseconds.
$(TEST_BIN): $(TEST_SRC) $(SRCDIR)/jalali.cpp $(SRCDIR)/gregorian.cpp $(SRCDIR)/holidays.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_SRC) $(SRCDIR)/jalali.cpp $(SRCDIR)/gregorian.cpp $(SRCDIR)/holidays.cpp $(LDFLAGS) $(LDLIBS)

# -----------------------------------------------------------------------------
# Housekeeping
# -----------------------------------------------------------------------------

clean:
	rm -f $(TARGET) $(OBJS) $(TEST_BIN)
	@echo "Cleaned."