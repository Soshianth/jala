# Makefile for jala
# Persian (Jalali) calendar in the terminal

TARGET      := jala
SRC         := jala.cpp
MANPAGE     := jala.1
PREFIX      ?= /usr/local
BINDIR      := $(PREFIX)/bin
MANDIR      := $(PREFIX)/share/man/man1

CXX         ?= g++
CXXFLAGS    ?= -std=c++17 -O2 -g -Wall -Wextra
LDFLAGS     ?=
LDLIBS      ?= -lboost_date_time

.PHONY: all clean install uninstall test debug

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

debug: CXXFLAGS += -DDEBUG -O0
debug: clean $(TARGET)

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

test: $(TARGET)
	@echo "Running tests..."
	@./tests/run_tests.sh

clean:
	rm -f $(TARGET) *.o
	@echo "Cleaned."
