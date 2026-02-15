define banner
         _                                                 
  _ __  (_)  ___    ___  __   __           ___   _ __ ___  
 | '__| | | / __|  / __| \ \ / /  _____   / _ \ | '_ ` _ \ 
 | |    | | \__ \ | (__   \ V /  |_____| |  __/ | | | | | |
 |_|    |_| |___/  \___|   \_/            \___| |_| |_| |_|
                                                           
endef

export banner

GIT_BRANCH := $(shell git rev-parse --abbrev-ref HEAD)
GIT_HASH := $(shell git rev-parse HEAD)

ifeq ($(GIT_BRANCH),)
    GIT_BRANCH := unknown
endif
ifeq ($(GIT_HASH),)
    GIT_HASH := unknown
endif

ifdef CROSS_COMPILE
	CXX := $(CROSS_COMPILE)g++
	LD := $(CROSS_COMPILE)ld
endif

CXX ?= g++
LD ?= ld

TRIPLET := $(shell $(CXX) -dumpmachine)

EMPTY := 
SPACE := $(EMPTY) $(EMPTY)
TRIPLET_WORDS := $(subst -,$(SPACE),$(TRIPLET))
TRIPLET_ARCH  := $(word 1,$(TRIPLET_WORDS))
TRIPLET_2     := $(word 2,$(TRIPLET_WORDS))
TRIPLET_3     := $(word 3,$(TRIPLET_WORDS))
TRIPLET_4     := $(word 4,$(TRIPLET_WORDS))

ifeq ($(shell which $(CXX) 2>/dev/null),)
    $(error [FATAL] Compiler '$(CXX)' not found.)
endif
CXX_VERSION := $(shell $(CXX) --version | head -n 1)
LD_VERSION := $(shell $(LD) --version | head -n 1)

all:
	@echo "$$banner"
	@echo "Branch: $(GIT_BRANCH)-$(GIT_HASH)"
	@echo "Target arch: $(TRIPLET_ARCH)"
	@echo "Detected OS: $(shell uname -s)"
	@echo "Detected CXX: $(CXX_VERSION)"
	@echo "Detected LD: $(LD_VERSION)"
	@echo
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=DEBUG .. && make
clean:
	rm -rf build
