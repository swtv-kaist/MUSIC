#PARAMETERS
LLVM_BASE_PATH=/CS453/
LLVM_SRC_DIRECTORY_NAME=llvm
LLVM_BUILD_DIRECTORY_NAME=llvm_build
LLVM_BUILD_MODE=Debug+Asserts
#LLVM_BUILD_MODE=Release+Asserts

SRCS=tool.cpp configuration.cpp mutant_operator.cpp comut_utility.cpp \
		 mutant_operator_holder.cpp mutation_operators/mutant_operator_template.cpp \
		 mutation_operators/ssdl.cpp mutation_operators/orrn.cpp \
		 mutation_operators/vtwf.cpp mutation_operators/crcr.cpp \
		 mutation_operators/sanl.cpp mutation_operators/srws.cpp \
		 mutation_operators/scsr.cpp mutation_operators/vlsf.cpp \
		 mutation_operators/vgsf.cpp mutation_operators/vltf.cpp \
		 mutation_operators/vgtf.cpp mutation_operators/vlpf.cpp \
		 mutation_operators/vgpf.cpp mutation_operators/vgsr.cpp \
		 mutation_operators/vlsr.cpp mutation_operators/vgar.cpp \
		 mutation_operators/vlar.cpp mutation_operators/vgtr.cpp \
		 mutation_operators/vltr.cpp mutation_operators/vgpr.cpp \
		 mutation_operators/vlpr.cpp mutation_operators/vtwd.cpp \
		 mutation_operators/vscr.cpp mutation_operators/cgcr.cpp \
		 mutation_operators/clcr.cpp mutation_operators/cgsr.cpp \
		 mutation_operators/clsr.cpp mutation_operators/oppo.cpp \
		 mutation_operators/ommo.cpp mutation_operators/olng.cpp \
		 mutation_operators/obng.cpp mutation_operators/ocng.cpp \
		 mutation_operators/oipm.cpp

OBJS=tool.o configuration.o mutant_operator.o comut_utility.o \
		 mutant_operator_holder.o mutant_operator_template.o ssdl.o \
		 orrn.o vtwf.o crcr.o sanl.o srws.o scsr.o vlsf.o vgsf.o \
		 vltf.o vgtf.o vlpf.o vgpf.o vgsr.o vlsr.o vgar.o vlar.o \
		 vgtr.o vltr.o vgpr.o vlpr.o vtwd.o vscr.o cgcr.o clcr.o \
		 cgsr.o clsr.o oppo.o ommo.o olng.o obng.o ocng.o oipm.o

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

tool.o : tool.cpp comut_utility.h configuration.h mutant_operator.h comut_context.h\
	mutant_operator_holder.h mutation_operators/mutant_operator_template.h \
	mutation_operators/ssdl.h mutation_operators/orrn.h mutation_operators/vtwf.h \
	mutation_operators/crcr.h mutation_operators/sanl.h mutation_operators/srws.h \
	mutation_operators/scsr.h mutation_operators/vlsf.h mutation_operators/vgsf.h \
	mutation_operators/vltf.h mutation_operators/vgtf.h mutation_operators/vlpf.h \
	mutation_operators/vgpf.h mutation_operators/vgsr.h mutation_operators/vlsr.h \
	mutation_operators/vgar.h mutation_operators/vlar.h mutation_operators/vgtr.h \
	mutation_operators/vltr.h mutation_operators/vgpr.h mutation_operators/vlpr.h \
	mutation_operators/vtwd.h mutation_operators/vscr.h mutation_operators/cgcr.h \
	mutation_operators/clcr.h mutation_operators/cgsr.h mutation_operators/clsr.h \
	mutation_operators/oppo.h mutation_operators/ommo.h mutation_operators/olng.h \
	mutation_operators/obng.h mutation_operators/ocng.h mutation_operators/oipm.h
	$(CXX) $(CXXFLAGS) -c tool.cpp

configuration.o : configuration.h configuration.cpp
	$(CXX) $(CXXFLAGS) -c configuration.cpp

mutant_operator.o : mutant_operator.h mutant_operator.cpp
	$(CXX) $(CXXFLAGS) -c mutant_operator.cpp

comut_utility.o : comut_utility.h comut_utility.cpp
	$(CXX) $(CXXFLAGS) -c comut_utility.cpp

mutant_operator_holder.o : mutant_operator_holder.h mutant_operator_holder.cpp \
	mutant_operator.h comut_utility.h
	$(CXX) $(CXXFLAGS) -c mutant_operator_holder.cpp

mutant_operator_template.o : mutation_operators/mutant_operator_template.h mutation_operators/mutant_operator_template.cpp comut_utility.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/mutant_operator_template.cpp

ssdl.o : mutation_operators/ssdl.h mutation_operators/ssdl.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/ssdl.cpp

orrn.o : mutation_operators/orrn.h mutation_operators/orrn.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/orrn.cpp

vtwf.o : mutation_operators/vtwf.h mutation_operators/vtwf.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vtwf.cpp

crcr.o : mutation_operators/crcr.h mutation_operators/crcr.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/crcr.cpp

sanl.o : mutation_operators/sanl.h mutation_operators/sanl.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/sanl.cpp

srws.o : mutation_operators/srws.h mutation_operators/srws.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/srws.cpp

scsr.o : mutation_operators/scsr.h mutation_operators/scsr.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/scsr.cpp

vlsf.o : mutation_operators/vlsf.h mutation_operators/vlsf.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vlsf.cpp

vgsf.o : mutation_operators/vgsf.h mutation_operators/vgsf.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vgsf.cpp

vltf.o : mutation_operators/vltf.h mutation_operators/vltf.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vltf.cpp

vgtf.o : mutation_operators/vgtf.h mutation_operators/vgtf.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vgtf.cpp

vlpf.o : mutation_operators/vlpf.h mutation_operators/vlpf.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vlpf.cpp

vgpf.o : mutation_operators/vgpf.h mutation_operators/vgpf.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vgpf.cpp

vgsr.o : mutation_operators/vgsr.h mutation_operators/vgsr.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vgsr.cpp

vlsr.o : mutation_operators/vlsr.h mutation_operators/vlsr.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vlsr.cpp

vgar.o : mutation_operators/vgar.h mutation_operators/vgar.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vgar.cpp

vlar.o : mutation_operators/vlar.h mutation_operators/vlar.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vlar.cpp

vgtr.o : mutation_operators/vgtr.h mutation_operators/vgtr.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vgtr.cpp

vltr.o : mutation_operators/vltr.h mutation_operators/vltr.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vltr.cpp

vgpr.o : mutation_operators/vgpr.h mutation_operators/vgpr.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vgpr.cpp

vlpr.o : mutation_operators/vlpr.h mutation_operators/vlpr.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vlpr.cpp

vtwd.o : mutation_operators/vtwd.h mutation_operators/vtwd.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vtwd.cpp

vscr.o : mutation_operators/vscr.h mutation_operators/vscr.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vscr.cpp

cgcr.o : mutation_operators/cgcr.h mutation_operators/cgcr.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/cgcr.cpp

clcr.o : mutation_operators/clcr.h mutation_operators/clcr.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/clcr.cpp

cgsr.o : mutation_operators/cgsr.h mutation_operators/cgsr.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/cgsr.cpp

clsr.o : mutation_operators/clsr.h mutation_operators/clsr.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/clsr.cpp

oppo.o : mutation_operators/oppo.h mutation_operators/oppo.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oppo.cpp

ommo.o : mutation_operators/ommo.h mutation_operators/ommo.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/ommo.cpp

olng.o : mutation_operators/olng.h mutation_operators/olng.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/olng.cpp

obng.o : mutation_operators/obng.h mutation_operators/obng.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/obng.cpp

ocng.o : mutation_operators/ocng.h mutation_operators/ocng.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/ocng.cpp

oipm.o : mutation_operators/oipm.h mutation_operators/oipm.cpp \
	mutation_operators/mutant_operator_template.h comut_utility.h \
	comut_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oipm.cpp

clean:
	rm -rf $(OBJS)
