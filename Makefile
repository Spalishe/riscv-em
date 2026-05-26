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
GIT_HASH_SHORT := $(shell git rev-parse --short HEAD)

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

LIBS := -latomic -pthread
CXXFLAGS := -std=c++20 -std=gnu++20 $(LIBS) -O3 -march=native -flto -MMD -MP -Iinclude

CXXFLAGS += $(foreach v,$(USE_VARS),$(if $(filter-out 0,$($(v))),-D$(v)=$($(v))))

CXXFLAGS += -DRVEM_VERSION='"riscv-em; git-$(GIT_HASH_SHORT)"'

BUILD_DIR := build.$(OS_lower).$(TRIPLET_ARCH)
OBJ_DIR_BIN := $(BUILD_DIR)/obj/bin/
OBJ_DIR_SO := $(BUILD_DIR)/obj/so/
SRCS := $(shell find src -name '*.cpp')
SRCS_SO := $(filter-out src/main.cpp, $(SRCS))
OBJS_BIN := $(patsubst src/%.cpp,$(OBJ_DIR_BIN)/%.o,$(SRCS))
OBJS_SO := $(patsubst src/%.cpp,$(OBJ_DIR_SO)/%.o,$(SRCS_SO))
TARGET_BIN := $(BUILD_DIR)/release_$(TRIPLET_ARCH)
TARGET_SO := $(BUILD_DIR)/lib_$(TRIPLET_ARCH).so

all:
	$(call print_info)
	
	@mkdir -p $(BUILD_DIR) 
	@mkdir -p $(OBJ_DIR_BIN) 
	
	@$(MAKE) --no-print-directory $(TARGET_BIN)

lib:
	$(call print_info)
	@mkdir -p $(BUILD_DIR) 
	@mkdir -p $(OBJ_DIR_SO) 

	@$(MAKE) --no-print-directory $(TARGET_SO)

help:
	$(call print_info)
	$(call print_help)
clean:
	$(call print_info)
	@rm -rf $(BUILD_DIR)
	
$(OBJ_DIR_BIN)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@echo -e "[$(ANSI_GREEN)CXX$(ANSI_RESET)] $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR_SO)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@echo -e "[$(ANSI_GREEN)CXX$(ANSI_RESET)] $<"
	@$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@

-include $(OBJS_BIN:.o=.d)
-include $(OBJS_SO:.o=.d)

$(TARGET_BIN): $(OBJS_BIN)
	@echo -e "[$(ANSI_BLUE)LD$(ANSI_RESET)] $@"
	@$(CXX) $(OBJS_BIN) -o $@ $(LIBS) $(LDFLAGS)

$(TARGET_SO): $(OBJS_SO)
	@echo -e "[$(ANSI_BLUE)LD$(ANSI_RESET)] $@"
	@$(CXX) $(OBJS_SO) -o $@ $(LIBS) -shared
