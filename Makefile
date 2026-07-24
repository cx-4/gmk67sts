CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -O2

PKGCONFIG := $(shell which pkg-config 2>/dev/null)
ifdef PKGCONFIG
  HIDAPI_CFLAGS := $(shell pkg-config --cflags hidapi-hidraw 2>/dev/null || \
                     pkg-config --cflags hidapi-libusb 2>/dev/null || \
                     pkg-config --cflags hidapi 2>/dev/null || echo "")
  HIDAPI_LIBS := $(shell pkg-config --libs hidapi-hidraw 2>/dev/null || \
                   pkg-config --libs hidapi-libusb 2>/dev/null || \
                   pkg-config --libs hidapi 2>/dev/null || echo "-lhidapi-hidraw")
endif
HIDAPI_CFLAGS := $(HIDAPI_CFLAGS)
HIDAPI_LIBS := $(HIDAPI_LIBS)

CFLAGS += $(HIDAPI_CFLAGS)
LDFLAGS = $(HIDAPI_LIBS)

SRCDIR = src
COREDIR = $(SRCDIR)/core
CLIDIR = $(SRCDIR)/cli
INCDIR = include
BUILDDIR = build
BINDIR = bin

CORE_SRCS = $(wildcard $(COREDIR)/*.c)
CLI_SRCS = $(wildcard $(CLIDIR)/*.c)
SRCS = $(CORE_SRCS) $(CLI_SRCS)
OBJS = $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
TARGET = $(BINDIR)/gmk67sts
INCLUDES = -I$(INCDIR) -I$(SRCDIR)

define check_hidapi
@echo "Checking dependencies..."
@if pkg-config --exists hidapi-hidraw 2>/dev/null || pkg-config --exists hidapi-libusb 2>/dev/null; then \
	echo "  hidapi: OK"; \
else \
	echo "  hidapi: NOT FOUND"; \
	echo "  Install: sudo apt-get install libhidapi-hidraw0 libhidapi-dev"; \
	exit 1; \
fi
endef

all: dirs $(TARGET)

dirs:
	$(check_hidapi)
	@mkdir -p $(BUILDDIR)/core $(BUILDDIR)/cli $(BINDIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/core/%.o: $(COREDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILDDIR)/cli/%.o: $(CLIDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/gmk67sts

install-service: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/gmk67sts
	install -m 644 deploy/gmk67sts.service /etc/systemd/system/gmk67sts.service
	systemctl daemon-reload
	@echo ""; echo "Service installed. Enable with: sudo systemctl enable --now gmk67sts"

uninstall-service:
	-systemctl stop gmk67sts 2>/dev/null || true
	-systemctl disable gmk67sts 2>/dev/null || true
	rm -f /etc/systemd/system/gmk67sts.service /usr/local/bin/gmk67sts
	systemctl daemon-reload

clean:
	rm -rf $(BUILDDIR) $(BINDIR)

run: $(TARGET)
	$(TARGET) --sync

debug: CFLAGS += -g -DDEBUG
debug: clean all

.PHONY: all dirs install install-service uninstall-service clean run debug