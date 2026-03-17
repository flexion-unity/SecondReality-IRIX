# Makefile for building on IRIX/IRIX64

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),IRIX)
 OS_IS_IRIX := 1
endif
ifneq ($(filter IRIX64,$(UNAME_S)),)
 OS_IS_IRIX := 1
endif
ifeq ($(OS_IS_IRIX),)
 $(error This build only supports IRIX or IRIX64)
endif

CXX := g++
CC  := gcc

ROOT_BUILD_DIR ?= _Intermediate
ROOT_OUTPUT_DIR ?= _Builds

BUILD_DIR   ?= $(ROOT_BUILD_DIR)/irix
OUTPUT_DIR  ?= $(ROOT_OUTPUT_DIR)/irix
CXXFLAGS    ?= -O3 -std=c++17 -Wall -Wextra -Wno-missing-braces -Wno-unused-function -pthread
CFLAGS      ?= -O3 -Wall -Wextra -Wno-missing-braces -Wno-unused-function

# SDL2-SGUG: Use system installed SDL2 from SGUG, but must support GL!!
#LDLIBS += $(shell sdl2-config --libs) -lGL -lGLcore /usr/lib32/libX11.so.1 /usr/lib32/libXext.a /usr/lib32/libXt.a /usr/lib32/libXm.so.1 /usr/lib32/libXpm.so.1 -lm -Wl,--allow-shlib-undefined -Wl,-rpath-link=/usr/lib32 -Wl,-rpath=/usr/lib32:/usr/sgug/lib32

# flx-SDL2 (GL) tdnf package: install flx-SDL2 or place libSDL2.so in ./lib
LDLIBS += -Wl,-rpath,'./lib:/usr/sgug-flx/lib32' /usr/sgug-flx/lib32/libSDL2.so -lGLcore /usr/lib32/libX11.so.1 /usr/lib32/libXext.a /usr/lib32/libXt.a /usr/lib32/libXm.so.1 /usr/lib32/libXpm.so.1 -lm -Wl,--allow-shlib-undefined -Wl,-rpath-link=/usr/lib32 -Wl,-rpath=./lib:/usr/lib32:/usr/sgug/lib32

OUT ?= secondreality
EXT ?=

SRC_DIR   ?= .
TARGET    := $(OUTPUT_DIR)/$(OUT)$(EXT)

CPPFLAGS += -I$(SRC_DIR)

CXXFLAGS += $(shell sdl2-config --cflags)

MKDIR_LINE = mkdir -p "$(@D)"
RM_RF      = rm -rf
WHICH     := which
NULLDEV   := /dev/null

CPP_EXTS := cpp cc cxx CPP C++
C_EXTS   := c

# IRIX-safe recursive wildcard replacement using find
CPP_SOURCES := $(shell find $(SRC_DIR) -type f \( -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" -o -name "*.CPP" -o -name "*.C++" \) | grep -v "/References/")
C_SOURCES   := $(shell find $(SRC_DIR) -type f -name "*.c" | grep -v "/References/")

# Exclude References directory
EXCLUDE_GLOBS := %/References/%
CPP_SOURCES   := $(filter-out $(EXCLUDE_GLOBS),$(CPP_SOURCES))
C_SOURCES     := $(filter-out $(EXCLUDE_GLOBS),$(C_SOURCES))

CPP_OBJECTS := $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(CPP_SOURCES))
C_OBJECTS   := $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(C_SOURCES))

# Normalize extensions to .o
OBJECTS := $(CPP_OBJECTS:.cpp=.o)
OBJECTS := $(OBJECTS:.cc=.o)
OBJECTS := $(OBJECTS:.cxx=.o)
OBJECTS := $(OBJECTS:.CPP=.o)
OBJECTS := $(OBJECTS:.C++=.o)
OBJECTS := $(OBJECTS:.c=.o)
OBJECTS := $(sort $(OBJECTS))

.PHONY: all clean help tree

all: $(TARGET)

help:
	@echo Targets:
	@echo   make tree : show sources/objects/target
	@echo   make clean : clean-up
	@echo.

tree:
	@echo "Sources (C++):"
	@$(foreach s,$(CPP_SOURCES),echo "  $(s)";)
	@echo "Sources (C):"
	@$(foreach s,$(C_SOURCES),echo "  $(s)";)
	@echo "Objects:"
	@$(foreach o,$(OBJECTS),echo "  $(o)";)
	@echo "Target: $(TARGET)"

print-%:
	@echo '$* = $($*)'

$(BUILD_DIR):
	@$(MKDIR_LINE)

$(ROOT_OUTPUT_DIR):
	@$(MKDIR_LINE)

$(OUTPUT_DIR):
	@$(MKDIR_LINE)

$(TARGET): $(OBJECTS) | $(OUTPUT_DIR)
	@$(MKDIR_LINE)
	$(CXX) -o "$@" $(OBJECTS) $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@$(MKDIR_LINE)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "$<" -o "$@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cc
	@$(MKDIR_LINE)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "$<" -o "$@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cxx
	@$(MKDIR_LINE)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "$<" -o "$@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.CPP
	@$(MKDIR_LINE)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "$<" -o "$@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.C++
	@$(MKDIR_LINE)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "$<" -o "$@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@$(MKDIR_LINE)
	$(CC)  $(CPPFLAGS) $(CFLAGS)   -c "$<" -o "$@"

clean:
	-@$(RM_RF) "$(ROOT_BUILD_DIR)" 2>$(NULLDEV)
	-@$(RM_RF) "$(ROOT_OUTPUT_DIR)" 2>$(NULLDEV)

