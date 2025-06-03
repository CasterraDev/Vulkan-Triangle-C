BUILD_DIR := bin
OBJ_DIR := obj

ASSEMBLY := engine
# EXTENSION := .so
EXTENSION :=
COMPILER_FLAGS := -g -MD -Werror=vla -fPIC -fdeclspec
INCLUDE_FLAGS := -Iengine/ -I$(VULKAN_SDK)/include
# LINKER_FLAGS := -g -shared -lvulkan -lX11 -lX11-xcb -lxcb -lxkbcommon -L$(VULKAN_SDK)/lib -L/usr/X11R6/lib -lm
LINKER_FLAGS := -g -lvulkan -lX11 -lX11-xcb -lxcb -lxkbcommon -L$(VULKAN_SDK)/lib -L/usr/X11R6/lib -lm
DEFINES := -D_DEBUG -DFSN_EXPORT

SRC_FILES := $(shell find $(ASSEMBLY) -name *.c)		# .c files
DIRECTORIES := $(shell find $(ASSEMBLY) -type d)		# directories with .h files
OBJ_FILES := $(SRC_FILES:%=$(OBJ_DIR)/%.o)		# compiled .o objects

all: build

.PHONY: build
build: scaffold compile link

.PHONY: scaffold
scaffold: # create build directory
	@echo Scaffolding folder structure...
	@mkdir -p $(BUILD_DIR)/
	@mkdir -p $(addprefix $(OBJ_DIR)/,$(DIRECTORIES))
	@echo Done.

.PHONY: link
link: scaffold $(OBJ_FILES) # link
	@echo Linking $(ASSEMBLY)...
	@echo LinkingV $(VULKAN_SDK)...
	@clang $(OBJ_FILES) -o $(BUILD_DIR)/$(ASSEMBLY)$(EXTENSION) $(LINKER_FLAGS) -v

.PHONY: compile
compile: #compile .c files
	@echo Compiling...

.PHONY: clean
clean: # clean build directory
	rm -rf $(BUILD_DIR)/$(ASSEMBLY)
	rm -rf $(OBJ_DIR)/$(ASSEMBLY)
	rm -rf $(BUILD_DIR)/lib$(ASSEMBLY)$(EXTENSION)
	rm -rf compile_commands.json

.PHONY: run
run:
	cd ./bin; ./engine

.PHONY: buildrun
buildrun: build run

$(OBJ_DIR)/%.c.o: %.c # compile .c to .o object
	@echo   $<...
	@clang $< $(COMPILER_FLAGS) -c -o $@ $(DEFINES) $(INCLUDE_FLAGS)

-include $(OBJ_FILES:.o=.d)
