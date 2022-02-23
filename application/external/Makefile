CC=clang
LD=clang

TARGETS		= snmpwalk onesixtyone
BINDIR		= ../bin/external
SRCDIR		= .

# shared library flags (assumes gcc)
DLFLAGS=-fPIC -shared

.PHONY: clean
.DEFAULT_GOAL := all

all: setup $(TARGETS)
setup: dir
remake: clean all

dir:
	@mkdir -p $(BINDIR)

onesixtyone:
	clang -o $(BINDIR)/$@ $(SRCDIR)/$@.c

snmpwalk:
	clang -o $(BINDIR)/$@ $(SRCDIR)/$@.c `net-snmp-config --agent-libs`

clean:
	rm -rf $(BINDIR)
