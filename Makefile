#Copyright 2026 Spalishe
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#

include objects.mk

define banner
         _                                                 
  _ __  (_)  ___    ___  __   __           ___   _ __ ___  
 | '__| | | / __|  / __| \ \ / /  _____   / _ \ | '_ ` _ \ 
 | |    | | \__ \ | (__   \ V /  |_____| |  __/ | | | | | |
 |_|    |_| |___/  \___|   \_/            \___| |_| |_| |_|
                                                           
endef

export banner

ANSI_GREEN := \x1b[32m
ANSI_BLUE := \x1b[34m
ANSI_RESET := \x1b[0m

define print_info
	@echo "$$banner"
	@echo -e "Branch: $(ANSI_GREEN)$(GIT_BRANCH)-$(GIT_HASH)$(ANSI_RESET)"
	@echo -e "Target arch: $(ANSI_GREEN)$(TRIPLET_ARCH)$(ANSI_RESET)"
	@echo -e "Detected OS: $(ANSI_GREEN)$(OS)$(ANSI_RESET)"
	@echo -e "Detected CXX: $(ANSI_GREEN)$(CXX_VERSION)$(ANSI_RESET)"
	@echo
endef

define print_help
	@echo -e "[$(ANSI_BLUE)INFO$(ANSI_RESET)] Available useflags:"
	@$(foreach var,$(USE_VARS),echo "  $(var)=$($(var))";)

	@echo
	@echo -e "[$(ANSI_BLUE)INFO$(ANSI_RESET)] Available commands:"
	@echo -e "  $(ANSI_GREEN)all$(ANSI_RESET)			Build target"
	@echo -e "  $(ANSI_GREEN)help$(ANSI_RESET)			Shows this menu"
	@echo -e "  $(ANSI_GREEN)clean$(ANSI_RESET)			Clean the build directory"
endef

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
endif

CXX ?= g++

TRIPLET := $(shell $(CXX) -dumpmachine)

OS = $(shell uname -s)
OS_lower = $(shell echo $(OS) | tr A-Z a-z)
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

LIBS := -latomic
CXXFLAGS := -std=c++20 $(LIBS) -O3 -Iinclude

CXXFLAGS += $(foreach v,$(USE_VARS),$(if $(filter-out 0,$($(v))),-D$(v)=$($(v))))

BUILD_DIR := build.$(OS_lower).$(TRIPLET_ARCH)
OBJ_DIR := $(BUILD_DIR)/obj
SRCS := $(shell find src -name '*.cpp')
OBJS := $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
TARGET := $(BUILD_DIR)/release_$(TRIPLET_ARCH)

all:
	$(call print_info)
	
	@mkdir -p $(BUILD_DIR) 
	@mkdir -p $(OBJ_DIR) 
	
	@$(MAKE) --no-print-directory $(TARGET)
help:
	$(call print_info)
	$(call print_help)
clean:
	$(call print_info)
	@rm -rf $(BUILD_DIR)
	
$(OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@echo -e "[$(ANSI_GREEN)CXX$(ANSI_RESET)] $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	@echo -e "[$(ANSI_BLUE)LD$(ANSI_RESET)] $@"
	@$(CXX) $(OBJS) -o $@ $(LIBS)