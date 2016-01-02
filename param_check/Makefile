# The name of the plugin.
PLUGIN = ParameterNameChecker

# Some LLVM paths.
# NOTE: You probably need to update these!
LLVM_DIR ?= ../../clang
CLANG_DIR = $(LLVM_DIR)/tools/clang
CLANG = $(LLVM_DIR)/bin/clang

# Some necessary flags.
CXXFLAGS  = -I$(LLVM_DIR)/include -I$(CLANG_DIR)/include
CXXFLAGS += -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -Wno-long-long
CXXFLAGS += -fPIC -fvisibility-inlines-hidden
CXXFLAGS += -fno-exceptions -fno-rtti
CXXFLAGS += -std=c++11 -stdlib=libc++

# Some warnings.
CXXFLAGS += -Wall -pedantic -Werror

# Linker flags.
LDFLAGS = -shared -Wl,-undefined,dynamic_lookup

$(PLUGIN).so : $(PLUGIN).o
	$(CXX) $(LDFLAGS) -o $(PLUGIN).so $(PLUGIN).o

$(PLUGIN).o : $(PLUGIN).cc
	$(CXX) $(CXXFLAGS) -c $(PLUGIN).cc -o $(PLUGIN).o

check : $(PLUGIN).so
	$(CLANG) -cc1 -fcolor-diagnostics \
		-load ./$(PLUGIN).so -plugin check-parameter-names test.c

clean :
	rm -fv $(PLUGIN).o $(PLUGIN).so
