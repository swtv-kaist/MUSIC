#PARAMETERS
LLVM_BASE_PATH=/CS453/
LLVM_SRC_DIRECTORY_NAME=llvm
LLVM_BUILD_DIRECTORY_NAME=llvm_build
LLVM_BUILD_MODE=Debug+Asserts
#LLVM_BUILD_MODE=Release+Asserts

SRCS=tool.cpp user_input.cpp mutant_operator.cpp comut_utility.cpp \
		 mutant_operator_holder.cpp
OBJS=tool.o user_input.o mutant_operator.o comut_utility.o \
		 mutant_operator_holder.o
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

$(TARGET) : $(OBJS)
	$(CXX) $(OBJS) $(CXXFLAGS) -o $@

tool.o : tool.cpp comut_utility.h user_input.h mutant_operator.h \
	mutant_operator_holder.h
	$(CXX) $(CXXFLAGS) -c tool.cpp

user_input.o : user_input.h user_input.cpp
	$(CXX) $(CXXFLAGS) -c user_input.cpp

mutant_operator.o : mutant_operator.h mutant_operator.cpp
	$(CXX) $(CXXFLAGS) -c mutant_operator.cpp

comut_utility.o : comut_utility.h comut_utility.cpp
	$(CXX) $(CXXFLAGS) -c comut_utility.cpp

mutant_operator_holder.o : mutant_operator_holder.h mutant_operator_holder.cpp \
	mutant_operator.h comut_utility.h
	$(CXX) $(CXXFLAGS) -c mutant_operator_holder.cpp

clean:
	rm -rf $(OBJS) $(TARGET)
