#PARAMETERS
LLVM_BASE_PATH=/CS453/
LLVM_SRC_DIRECTORY_NAME=llvm
LLVM_BUILD_DIRECTORY_NAME=llvm_build
LLVM_BUILD_MODE=Debug+Asserts
#LLVM_BUILD_MODE=Release+Asserts

OBJS=	tool.o 
SRCS=	tool.cpp 
TARGET=	tool

################
LLVM_SRC_PATH=$(LLVM_BASE_PATH)/$(LLVM_SRC_DIRECTORY_NAME)
LLVM_BUILD_PATH=$(LLVM_BASE_PATH)/$(LLVM_BUILD_DIRECTORY_NAME)

LLVM_BIN_PATH = $(LLVM_BUILD_PATH)/$(LLVM_BUILD_MODE)/bin
LLVM_LIBS=core mc all
LLVM_CONFIG_COMMAND = $(LLVM_BIN_PATH)/llvm-config  \
					  --ldflags --libs $(LLVM_LIBS)
CLANG_BUILD_FLAGS = -I$(LLVM_SRC_PATH)/tools/clang/include \
					-I$(LLVM_BUILD_PATH)/tools/clang/include

CLANGLIBS = \
	-lclangFrontendTool -lclangFrontend -lclangDriver \
	-lclangSerialization -lclangCodeGen -lclangParse \
	-lclangSema -lclangStaticAnalyzerFrontend \
	-lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore \
	-lclangAnalysis -lclangARCMigrate -lclangRewriteCore -lclangRewriteFrontend \
	-lclangEdit -lclangAST -lclangLex -lclangBasic

CXX=g++
CXX_INCLUDE=-I$(LLVM_SRC_PATH)/include -I$(LLVM_BUILD_PATH)/include
CXXFLAGS=$(CXX_INCLUDE) $(CLANG_BUILD_FLAGS) $(CLANGLIBS) `$(LLVM_CONFIG_COMMAND)` -fno-rtti -g -std=c++11 -O0 -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -fomit-frame-pointer -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual
		
all: $(TARGET)

$(TARGET):$(OBJS)
	$(CXX) $(OBJS) $(CXXFLAGS) -o $@ 

	
clean:
	rm -rf $(OBJS) $(TARGET)
