# Edit these variables based on where you put your llvm source file
# and build file.
# LLVM_SRC_PATH := /usr/lib/llvm-16
LLVM_BUILD_PATH := /usr/lib/llvm-16
LLVM_BIN_PATH := $(LLVM_BUILD_PATH)/bin

SRCS=tool.cpp configuration.cpp music_utility.cpp mutant_entry.cpp\
		 mutation_operators/mutant_operator_template.cpp \
		 information_visitor.cpp information_gatherer.cpp \
		 music_context.cpp music_ast_consumer.cpp \
		 symbol_table.cpp stmt_context.cpp mutant_database.cpp\
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
		 mutation_operators/oipm.cpp mutation_operators/ocor.cpp \
		 mutation_operators/olln.cpp mutation_operators/ossn.cpp \
		 mutation_operators/obbn.cpp mutation_operators/olrn.cpp \
		 mutation_operators/orln.cpp mutation_operators/obln.cpp \
		 mutation_operators/obrn.cpp mutation_operators/osln.cpp \
		 mutation_operators/osrn.cpp mutation_operators/oban.cpp \
		 mutation_operators/obsn.cpp mutation_operators/osan.cpp \
		 mutation_operators/osbn.cpp mutation_operators/oaea.cpp \
		 mutation_operators/obaa.cpp mutation_operators/obba.cpp \
		 mutation_operators/obea.cpp mutation_operators/obsa.cpp \
		 mutation_operators/osaa.cpp mutation_operators/osba.cpp \
		 mutation_operators/osea.cpp mutation_operators/ossa.cpp \
		 mutation_operators/oeaa.cpp mutation_operators/oeba.cpp \
		 mutation_operators/oesa.cpp mutation_operators/oaaa.cpp \
		 mutation_operators/oaba.cpp mutation_operators/oasa.cpp \
		 mutation_operators/oaln.cpp mutation_operators/oaan.cpp \
		 mutation_operators/oarn.cpp mutation_operators/oabn.cpp \
		 mutation_operators/oasn.cpp mutation_operators/olan.cpp \
		 mutation_operators/oran.cpp mutation_operators/olbn.cpp \
		 mutation_operators/olsn.cpp mutation_operators/orsn.cpp \
		 mutation_operators/orbn.cpp mutation_operators/rgcr.cpp \
		 mutation_operators/rlcr.cpp mutation_operators/rgsr.cpp \
		 mutation_operators/rlsr.cpp mutation_operators/rgpr.cpp \
		 mutation_operators/rlpr.cpp mutation_operators/rgtr.cpp \
		 mutation_operators/rltr.cpp mutation_operators/rgar.cpp \
		 mutation_operators/rlar.cpp mutation_operators/rrcr.cpp \
		 mutation_operators/dirvararineg.cpp mutation_operators/dirvarbitneg.cpp \
		 mutation_operators/dirvarlogneg.cpp mutation_operators/dirvarincdec.cpp \
		 mutation_operators/dirvarrepreq.cpp mutation_operators/dirvarrepcon.cpp \
		 mutation_operators/dirvarreppar.cpp mutation_operators/dirvarrepglo.cpp \
		 mutation_operators/dirvarrepext.cpp mutation_operators/dirvarreploc.cpp \
		 mutation_operators/indvararineg.cpp mutation_operators/indvarbitneg.cpp \
		 mutation_operators/indvarlogneg.cpp mutation_operators/indvarincdec.cpp \
		 mutation_operators/indvarrepreq.cpp mutation_operators/indvarrepcon.cpp \
		 mutation_operators/indvarreppar.cpp mutation_operators/indvarrepglo.cpp \
		 mutation_operators/indvarrepext.cpp mutation_operators/indvarreploc.cpp \
		 mutation_operators/sbrc.cpp mutation_operators/scrb.cpp \
		 mutation_operators/retstadel.cpp mutation_operators/argarineg.cpp \
		 argbitneg.cpp argdel.cpp argincdec.cpp arglogneg.cpp argrepreq.cpp \
		 argstcali.cpp funcaldel.cpp swdd.cpp sdwd.cpp vasm.cpp sglr.cpp ssom.cpp \
		 smvb.cpp srsr.cpp strp.cpp stri.cpp smtt.cpp smtc.cpp sswm.cpp vdtr.cpp \
		 sbrn.cpp scrn.cpp covallnod.cpp covalledg.cpp
  
OBJS=tool.o configuration.o music_utility.o symbol_table.o\
		 mutant_entry.o mutant_database.o \
		 stmt_context.o music_context.o mutant_operator_template.o \
		 information_visitor.o information_gatherer.o \
		 music_ast_consumer.o ssdl.o \
		 orrn.o vtwf.o crcr.o sanl.o srws.o scsr.o vlsf.o vgsf.o \
		 vltf.o vgtf.o vlpf.o vgpf.o vgsr.o vlsr.o vgar.o vlar.o \
		 vgtr.o vltr.o vgpr.o vlpr.o vtwd.o vscr.o cgcr.o clcr.o \
		 cgsr.o clsr.o oppo.o ommo.o olng.o obng.o ocng.o oipm.o \
		 ocor.o olln.o ossn.o obbn.o olrn.o orln.o obln.o obrn.o \
		 osln.o osrn.o oban.o obsn.o osan.o osbn.o oaea.o obaa.o \
		 obba.o obea.o obsa.o osaa.o osba.o osea.o ossa.o oeaa.o \
		 oeba.o oesa.o oaaa.o oaba.o oasa.o oaln.o oaan.o oarn.o \
		 oabn.o oasn.o olan.o oran.o olbn.o olsn.o orbn.o orsn.o \
		 rgcr.o rlcr.o rgsr.o rlsr.o rgpr.o rlpr.o rgtr.o rltr.o \
		 rgar.o rlar.o rrcr.o dirvararineg.o dirvarbitneg.o dirvarlogneg.o \
		 dirvarincdec.o dirvarrepreq.o dirvarrepcon.o dirvarreppar.o \
		 dirvarrepglo.o dirvarrepext.o dirvarreploc.o indvararineg.o \
		 indvarincdec.o indvarrepreq.o indvarrepcon.o indvarreppar.o \
		 indvarrepglo.o indvarrepext.o indvarreploc.o indvarlogneg.o \
		 indvarbitneg.o sbrc.o scrb.o retstadel.o argarineg.o argbitneg.o \
		 argdel.o argincdec.o arglogneg.o argrepreq.o argstcali.o \
		 argstcdif.o funcaldel.o swdd.o sdwd.o vasm.o sglr.o ssom.o smvb.o srsr.o \
		 strp.o stri.o smtt.o smtc.o sswm.o vdtr.o sbrn.o scrn.o covallnod.o \
		 covalledg.o

TARGET=	music

################
LLVM_LIBS := core mc all
LLVM_CONFIG_COMMAND := $(LLVM_BIN_PATH)/llvm-config  \
					  --ldflags --libs $(LLVM_LIBS)
CLANG_BUILD_FLAGS := -I$(LLVM_BUILD_PATH)/tools/clang/include

CLANG_LIBS := \
	-lclangTooling -lclangFrontendTool -lclangFrontend -lclangDriver \
	-lclangSerialization -lclangCodeGen -lclangParse \
	-lclangSema -lclangStaticAnalyzerFrontend \
	-lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore \
	-lclangAnalysis -lclangARCMigrate -lclangRewrite \
	-lclangRewriteFrontend -lclangEdit -lclangAST -lclangASTMatchers \
	-lclangLex -lclangBasic -lclangSupport -pthread

CXX := g++

CLANG_INCLUDES := -I$(LLVM_BUILD_PATH)/include

CXXFLAGS := $(CLANG_INCLUDES) $(CLANG_BUILD_FLAGS) $(CLANG_LIBS) `$(LLVM_CONFIG_COMMAND)` -fno-rtti -g -std=c++17 -O0 -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -fomit-frame-pointer -fvisibility-inlines-hidden -fexceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual -ldl -lz -ltinfo
		
all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) $(OBJS) $(CXXFLAGS) -o $@

tool.o : tool.cpp music_utility.h configuration.h music_context.h \
	information_visitor.h information_gatherer.h symbol_table.h stmt_context.h \
	mutant_entry.h mutant_database.h all_mutant_operators.h music_ast_consumer.h \
	music_context.h mutation_operators/mutant_operator_template.h \
	mutation_operators/expr_mutant_operator.h mutation_operators/stmt_mutant_operator.h \
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
	mutation_operators/obng.h mutation_operators/ocng.h mutation_operators/oipm.h \
	mutation_operators/ocor.h mutation_operators/olln.h mutation_operators/ossn.h \
	mutation_operators/obbn.h mutation_operators/olrn.h mutation_operators/orln.h \
	mutation_operators/obln.h mutation_operators/obrn.h mutation_operators/osln.h \
	mutation_operators/osrn.h mutation_operators/oban.h mutation_operators/obsn.h \
	mutation_operators/osan.h mutation_operators/osbn.h mutation_operators/oaea.h \
	mutation_operators/obaa.h mutation_operators/obba.h mutation_operators/obea.h \
	mutation_operators/obsa.h mutation_operators/osaa.h mutation_operators/osba.h \
	mutation_operators/osea.h mutation_operators/ossa.h mutation_operators/ossa.h \
	mutation_operators/oeba.h mutation_operators/oesa.h mutation_operators/oaaa.h \
	mutation_operators/oaba.h mutation_operators/oasa.h mutation_operators/oaln.h \
	mutation_operators/oaan.h mutation_operators/oarn.h mutation_operators/oabn.h \
	mutation_operators/oasn.h mutation_operators/olan.h mutation_operators/oran.h \
	mutation_operators/olbn.h mutation_operators/olsn.h mutation_operators/orsn.h \
	mutation_operators/orbn.h mutation_operators/rgcr.h mutation_operators/rlcr.h \
	mutation_operators/rgsr.h mutation_operators/rlsr.h mutation_operators/rgpr.h \
	mutation_operators/rlpr.h mutation_operators/rgtr.h mutation_operators/rltr.h \
	mutation_operators/rgar.h mutation_operators/rlar.h mutation_operators/rrcr.h \
	mutation_operators/dirvararineg.h mutation_operators/dirvarbitneg.h \
	mutation_operators/dirvarlogneg.h mutation_operators/dirvarincdec.h \
	mutation_operators/dirvarrepreq.h mutation_operators/dirvarrepcon.h \
	mutation_operators/dirvarreppar.h mutation_operators/dirvarrepglo.h \
	mutation_operators/dirvarrepext.h mutation_operators/dirvarreploc.h \
	mutation_operators/indvararineg.h mutation_operators/indvarbitneg.h \
	mutation_operators/indvarlogneg.h mutation_operators/indvarincdec.h \
	mutation_operators/indvarrepreq.h mutation_operators/indvarrepcon.h \
	mutation_operators/indvarreppar.h mutation_operators/indvarrepglo.h \
	mutation_operators/indvarrepext.h mutation_operators/indvarreploc.h \
	mutation_operators/sbrc.h mutation_operators/scrb.h \
	mutation_operators/retstadel.h mutation_operators/argarineg.h \
	mutation_operators/argbitneg.h mutation_operators/argdel.h \
	mutation_operators/arglogneg.h mutation_operators/argincdec.h \
	mutation_operators/argrepreq.h mutation_operators/argstcali.h \
	mutation_operators/argstcdif.h mutation_operators/funcaldel.h \
	mutation_operators/swdd.h mutation_operators/sdwd.h mutation_operators/vasm.h \
	mutation_operators/sglr.h mutation_operators/ssom.h mutation_operators/smvb.h \
	mutation_operators/srsr.h mutation_operators/strp.h mutation_operators/stri.h \
	mutation_operators/smtt.h mutation_operators/smtc.h mutation_operators/sswm.h \
	mutation_operators/vdtr.h mutation_operators/sbrn.h mutation_operators/scrn.h \
	mutation_operators/covallnod.h mutation_operators/covalledg.h

	$(CXX) $(CXXFLAGS) -c tool.cpp

configuration.o : configuration.h configuration.cpp \
	mutation_operators/expr_mutant_operator.h mutation_operators/stmt_mutant_operator.h
	$(CXX) $(CXXFLAGS) -c configuration.cpp

music_utility.o : music_utility.h music_utility.cpp mutant_database.h
	$(CXX) $(CXXFLAGS) -c music_utility.cpp

information_visitor.o : information_visitor.h information_visitor.cpp \
	music_context.h music_utility.h
	$(CXX) $(CXXFLAGS) -c information_visitor.cpp

information_gatherer.o : information_gatherer.h information_gatherer.cpp \
	music_context.h information_visitor.h
	$(CXX) $(CXXFLAGS) -c information_gatherer.cpp

symbol_table.o: symbol_table.h symbol_table.cpp 
	$(CXX) $(CXXFLAGS) -c symbol_table.cpp

mutant_entry.o: mutant_entry.h mutant_entry.cpp music_utility.h
	$(CXX) $(CXXFLAGS) -c mutant_entry.cpp

mutant_database.o: mutant_database.h mutant_database.cpp mutant_entry.h \
	music_utility.h
	$(CXX) $(CXXFLAGS) -c mutant_database.cpp

stmt_context.o: stmt_context.h stmt_context.cpp music_utility.h
	$(CXX) $(CXXFLAGS) -c stmt_context.cpp

music_context.o : music_context.h music_context.cpp configuration.h \
	symbol_table.h stmt_context.h
	$(CXX) $(CXXFLAGS) -c music_context.cpp

mutant_operator_template.o : mutation_operators/mutant_operator_template.h \
	mutation_operators/mutant_operator_template.cpp music_utility.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/mutant_operator_template.cpp

music_ast_consumer.o: music_ast_consumer.h music_ast_consumer.cpp \
	mutation_operators/expr_mutant_operator.h mutation_operators/stmt_mutant_operator.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c music_ast_consumer.cpp

ssdl.o : mutation_operators/ssdl.h mutation_operators/ssdl.cpp \
	mutation_operators/stmt_mutant_operator.h music_utility.h \
	music_context.h mutation_operators/mutant_operator_template.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/ssdl.cpp

orrn.o : mutation_operators/orrn.h mutation_operators/orrn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/orrn.cpp

vtwf.o : mutation_operators/vtwf.h mutation_operators/vtwf.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vtwf.cpp

crcr.o : mutation_operators/crcr.h mutation_operators/crcr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/crcr.cpp

sanl.o : mutation_operators/sanl.h mutation_operators/sanl.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/sanl.cpp

srws.o : mutation_operators/srws.h mutation_operators/srws.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/srws.cpp

scsr.o : mutation_operators/scsr.h mutation_operators/scsr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/scsr.cpp

vlsf.o : mutation_operators/vlsf.h mutation_operators/vlsf.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vlsf.cpp

vgsf.o : mutation_operators/vgsf.h mutation_operators/vgsf.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vgsf.cpp

vltf.o : mutation_operators/vltf.h mutation_operators/vltf.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vltf.cpp

vgtf.o : mutation_operators/vgtf.h mutation_operators/vgtf.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vgtf.cpp

vlpf.o : mutation_operators/vlpf.h mutation_operators/vlpf.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vlpf.cpp

vgpf.o : mutation_operators/vgpf.h mutation_operators/vgpf.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vgpf.cpp

vgsr.o : mutation_operators/vgsr.h mutation_operators/vgsr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vgsr.cpp

vlsr.o : mutation_operators/vlsr.h mutation_operators/vlsr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vlsr.cpp

vgar.o : mutation_operators/vgar.h mutation_operators/vgar.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vgar.cpp

vlar.o : mutation_operators/vlar.h mutation_operators/vlar.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vlar.cpp

vgtr.o : mutation_operators/vgtr.h mutation_operators/vgtr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vgtr.cpp

vltr.o : mutation_operators/vltr.h mutation_operators/vltr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vltr.cpp

vgpr.o : mutation_operators/vgpr.h mutation_operators/vgpr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vgpr.cpp

vlpr.o : mutation_operators/vlpr.h mutation_operators/vlpr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vlpr.cpp

vtwd.o : mutation_operators/vtwd.h mutation_operators/vtwd.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vtwd.cpp

vscr.o : mutation_operators/vscr.h mutation_operators/vscr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vscr.cpp

cgcr.o : mutation_operators/cgcr.h mutation_operators/cgcr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/cgcr.cpp

clcr.o : mutation_operators/clcr.h mutation_operators/clcr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/clcr.cpp

cgsr.o : mutation_operators/cgsr.h mutation_operators/cgsr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/cgsr.cpp

clsr.o : mutation_operators/clsr.h mutation_operators/clsr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/clsr.cpp

oppo.o : mutation_operators/oppo.h mutation_operators/oppo.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oppo.cpp

ommo.o : mutation_operators/ommo.h mutation_operators/ommo.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/ommo.cpp

olng.o : mutation_operators/olng.h mutation_operators/olng.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/olng.cpp

obng.o : mutation_operators/obng.h mutation_operators/obng.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/obng.cpp

ocng.o : mutation_operators/ocng.h mutation_operators/ocng.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h mutation_operators/stmt_mutant_operator.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/ocng.cpp

oipm.o : mutation_operators/oipm.h mutation_operators/oipm.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h stmt_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oipm.cpp

ocor.o : mutation_operators/ocor.h mutation_operators/ocor.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/ocor.cpp

olln.o : mutation_operators/olln.h mutation_operators/olln.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/olln.cpp


ossn.o : mutation_operators/ossn.h mutation_operators/ossn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/ossn.cpp

obbn.o : mutation_operators/obbn.h mutation_operators/obbn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/obbn.cpp

olrn.o : mutation_operators/olrn.h mutation_operators/olrn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/olrn.cpp

orln.o : mutation_operators/orln.h mutation_operators/orln.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/orln.cpp

obln.o : mutation_operators/obln.h mutation_operators/obln.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/obln.cpp

obrn.o : mutation_operators/obrn.h mutation_operators/obrn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/obrn.cpp

osln.o : mutation_operators/osln.h mutation_operators/osln.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/osln.cpp

osrn.o : mutation_operators/osrn.h mutation_operators/osrn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/osrn.cpp

oban.o : mutation_operators/oban.h mutation_operators/oban.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oban.cpp

obsn.o : mutation_operators/obsn.h mutation_operators/obsn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/obsn.cpp

osan.o : mutation_operators/osan.h mutation_operators/osan.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/osan.cpp

osbn.o : mutation_operators/osbn.h mutation_operators/osbn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/osbn.cpp

oaea.o : mutation_operators/oaea.h mutation_operators/oaea.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oaea.cpp

obaa.o : mutation_operators/obaa.h mutation_operators/obaa.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/obaa.cpp

obba.o : mutation_operators/obba.h mutation_operators/obba.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/obba.cpp

obea.o : mutation_operators/obea.h mutation_operators/obea.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/obea.cpp

obsa.o : mutation_operators/obsa.h mutation_operators/obsa.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/obsa.cpp

osaa.o : mutation_operators/osaa.h mutation_operators/osaa.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/osaa.cpp

osba.o : mutation_operators/osba.h mutation_operators/osba.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/osba.cpp

osea.o : mutation_operators/osea.h mutation_operators/osea.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/osea.cpp

ossa.o : mutation_operators/ossa.h mutation_operators/ossa.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/ossa.cpp

oeaa.o : mutation_operators/oeaa.h mutation_operators/oeaa.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oeaa.cpp

oeba.o : mutation_operators/oeba.h mutation_operators/oeba.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oeba.cpp

oesa.o : mutation_operators/oesa.h mutation_operators/oesa.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oesa.cpp

oaaa.o : mutation_operators/oaaa.h mutation_operators/oaaa.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oaaa.cpp

oaba.o : mutation_operators/oaba.h mutation_operators/oaba.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oaba.cpp

oasa.o : mutation_operators/oasa.h mutation_operators/oasa.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oasa.cpp

oaln.o : mutation_operators/oaln.h mutation_operators/oaln.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oaln.cpp

oaan.o : mutation_operators/oaan.h mutation_operators/oaan.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oaan.cpp

oarn.o : mutation_operators/oarn.h mutation_operators/oarn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oarn.cpp

oabn.o : mutation_operators/oabn.h mutation_operators/oabn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oabn.cpp

oasn.o : mutation_operators/oasn.h mutation_operators/oasn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oasn.cpp

olan.o : mutation_operators/olan.h mutation_operators/olan.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/olan.cpp

oran.o : mutation_operators/oran.h mutation_operators/oran.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/oran.cpp

olbn.o : mutation_operators/olbn.h mutation_operators/olbn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/olbn.cpp

olsn.o : mutation_operators/olsn.h mutation_operators/olsn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/olsn.cpp

orsn.o : mutation_operators/orsn.h mutation_operators/orsn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/orsn.cpp

orbn.o : mutation_operators/orbn.h mutation_operators/orbn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/orbn.cpp

rgcr.o : mutation_operators/rgcr.h mutation_operators/rgcr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/rgcr.cpp

rlcr.o : mutation_operators/rlcr.h mutation_operators/rlcr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/rlcr.cpp

rgsr.o : mutation_operators/rgsr.h mutation_operators/rgsr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/rgsr.cpp

rlsr.o : mutation_operators/rlsr.h mutation_operators/rlsr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/rlsr.cpp

rgpr.o : mutation_operators/rgpr.h mutation_operators/rgpr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/rgpr.cpp

rlpr.o : mutation_operators/rlpr.h mutation_operators/rlpr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/rlpr.cpp

rgtr.o : mutation_operators/rgtr.h mutation_operators/rgtr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/rgtr.cpp

rltr.o : mutation_operators/rltr.h mutation_operators/rltr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/rltr.cpp

rgar.o : mutation_operators/rgar.h mutation_operators/rgar.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/rgar.cpp

rlar.o : mutation_operators/rlar.h mutation_operators/rlar.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/rlar.cpp

rrcr.o : mutation_operators/rrcr.h mutation_operators/rrcr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/rrcr.cpp

dirvararineg.o : mutation_operators/dirvararineg.h mutation_operators/dirvararineg.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/dirvararineg.cpp

dirvarbitneg.o : mutation_operators/dirvarbitneg.h mutation_operators/dirvarbitneg.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/dirvarbitneg.cpp

dirvarlogneg.o : mutation_operators/dirvarlogneg.h mutation_operators/dirvarlogneg.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/dirvarlogneg.cpp

dirvarincdec.o : mutation_operators/dirvarincdec.h mutation_operators/dirvarincdec.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/dirvarincdec.cpp

dirvarrepreq.o : mutation_operators/dirvarrepreq.h mutation_operators/dirvarrepreq.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/dirvarrepreq.cpp

dirvarrepcon.o : mutation_operators/dirvarrepcon.h mutation_operators/dirvarrepcon.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/dirvarrepcon.cpp

dirvarreppar.o : mutation_operators/dirvarreppar.h mutation_operators/dirvarreppar.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/dirvarreppar.cpp

dirvarrepglo.o : mutation_operators/dirvarrepglo.h mutation_operators/dirvarrepglo.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/dirvarrepglo.cpp

dirvarrepext.o : mutation_operators/dirvarrepext.h mutation_operators/dirvarrepext.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/dirvarrepext.cpp

dirvarreploc.o : mutation_operators/dirvarreploc.h mutation_operators/dirvarreploc.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/dirvarreploc.cpp

indvararineg.o : mutation_operators/indvararineg.h mutation_operators/indvararineg.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/indvararineg.cpp

indvarbitneg.o : mutation_operators/indvarbitneg.h mutation_operators/indvarbitneg.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/indvarbitneg.cpp

indvarlogneg.o : mutation_operators/indvarlogneg.h mutation_operators/indvarlogneg.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/indvarlogneg.cpp

indvarincdec.o : mutation_operators/indvarincdec.h mutation_operators/indvarincdec.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/indvarincdec.cpp

indvarrepreq.o : mutation_operators/indvarrepreq.h mutation_operators/indvarrepreq.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/indvarrepreq.cpp

indvarrepcon.o : mutation_operators/indvarrepcon.h mutation_operators/indvarrepcon.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/indvarrepcon.cpp

indvarreppar.o : mutation_operators/indvarreppar.h mutation_operators/indvarreppar.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/indvarreppar.cpp

indvarrepglo.o : mutation_operators/indvarrepglo.h mutation_operators/indvarrepglo.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/indvarrepglo.cpp

indvarrepext.o : mutation_operators/indvarrepext.h mutation_operators/indvarrepext.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/indvarrepext.cpp

indvarreploc.o : mutation_operators/indvarreploc.h mutation_operators/indvarreploc.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/indvarreploc.cpp

sbrc.o : mutation_operators/sbrc.h mutation_operators/sbrc.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/sbrc.cpp

scrb.o : mutation_operators/scrb.h mutation_operators/scrb.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/scrb.cpp

retstadel.o : mutation_operators/retstadel.h mutation_operators/retstadel.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/retstadel.cpp

argarineg.o : mutation_operators/argarineg.h mutation_operators/argarineg.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/argarineg.cpp

argbitneg.o : mutation_operators/argbitneg.h mutation_operators/argbitneg.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/argbitneg.cpp

argdel.o : mutation_operators/argdel.h mutation_operators/argdel.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/argdel.cpp

argincdec.o : mutation_operators/argincdec.h mutation_operators/argincdec.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/argincdec.cpp

arglogneg.o : mutation_operators/arglogneg.h mutation_operators/arglogneg.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/arglogneg.cpp

argrepreq.o : mutation_operators/argrepreq.h mutation_operators/argrepreq.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/argrepreq.cpp

argstcali.o : mutation_operators/argstcali.h mutation_operators/argstcali.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/argstcali.cpp

argstcdif.o : mutation_operators/argstcdif.h mutation_operators/argstcdif.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/argstcdif.cpp

funcaldel.o : mutation_operators/funcaldel.h mutation_operators/funcaldel.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/funcaldel.cpp

swdd.o : mutation_operators/swdd.h mutation_operators/swdd.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/swdd.cpp	

sdwd.o : mutation_operators/sdwd.h mutation_operators/sdwd.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/sdwd.cpp	

vasm.o : mutation_operators/vasm.h mutation_operators/vasm.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vasm.cpp	

sglr.o : mutation_operators/sglr.h mutation_operators/sglr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/sglr.cpp	

ssom.o : mutation_operators/ssom.h mutation_operators/ssom.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/ssom.cpp	

smvb.o : mutation_operators/smvb.h mutation_operators/smvb.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/smvb.cpp	

srsr.o : mutation_operators/srsr.h mutation_operators/srsr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/srsr.cpp	

strp.o : mutation_operators/strp.h mutation_operators/strp.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/strp.cpp	

stri.o : mutation_operators/stri.h mutation_operators/stri.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/stri.cpp	

smtt.o : mutation_operators/smtt.h mutation_operators/smtt.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/smtt.cpp	

smtc.o : mutation_operators/smtc.h mutation_operators/smtc.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/smtc.cpp	

sswm.o : mutation_operators/sswm.h mutation_operators/sswm.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/sswm.cpp	

vdtr.o : mutation_operators/vdtr.h mutation_operators/vdtr.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/vdtr.cpp	

sbrn.o : mutation_operators/sbrn.h mutation_operators/sbrn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/sbrn.cpp	

scrn.o : mutation_operators/scrn.h mutation_operators/scrn.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/scrn.cpp	

covallnod.o : mutation_operators/covallnod.h mutation_operators/covallnod.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/covallnod.cpp	

covalledg.o : mutation_operators/covalledg.h mutation_operators/covalledg.cpp \
	mutation_operators/mutant_operator_template.h music_utility.h \
	music_context.h
	$(CXX) $(CXXFLAGS) -c mutation_operators/covalledg.cpp	

clean:
	rm -rf $(OBJS)
