# =============================================================================
# Makefile for jala
# Persian (Jalali) calendar in the terminal
# =============================================================================

# =============================================================================
# Configuration
# =============================================================================

TARGET      := jala
SRCDIR      := src
SRCS        := $(SRCDIR)/main.cpp $(SRCDIR)/jalali.cpp $(SRCDIR)/holidays.cpp
HDRS        := $(SRCDIR)/jalali.hpp
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
LDLIBS      ?= -lboost_date_time

# =============================================================================
# Targets
# =============================================================================

.PHONY: all clean install uninstall test test-unit debug

all: $(TARGET)

# -----------------------------------------------------------------------------
# Main binary
# -----------------------------------------------------------------------------

# Link the executable from all object files
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

# Compile each source file to an object file. The header is listed as a
# dependency so any change to jalali.hpp triggers a full rebuild.
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

# The unit tests link against jalali.cpp directly, with no dependency on
# main.cpp or the CLI. They run in a few milliseconds.
$(TEST_BIN): $(TEST_SRC) $(SRCDIR)/jalali.cpp $(SRCDIR)/holidays.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_SRC) $(SRCDIR)/jalali.cpp $(SRCDIR)/holidays.cpp $(LDFLAGS) $(LDLIBS)

# -----------------------------------------------------------------------------
# Housekeeping
# -----------------------------------------------------------------------------

clean:
	rm -f $(TARGET) $(OBJS) $(TEST_BIN)
	@echo "Cleaned."