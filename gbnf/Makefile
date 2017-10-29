#mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
#current_dir := $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))

CXX = g++
AR = ar

CXXFLAGS= -std=c++11
LDFLAGS= 

DEBUG_CXXFLAGS= -g
RELEASE_CXXFLAGS= -O2

TESTDIR= ./bin/test
LIBDIR= ./lib
INCLUDEDIR= ./include
BINDIR= ./bin

BINDIR_RELEASE= $(BINDIR)/release
BINDIR_DEBUG= $(BINDIR)/debug

#====================================#
#set directories
ZSH_RESULT:=$(shell mkdir -p $(TESTDIR) $(LIBDIR) $(BINDIR_RELEASE) $(BINDIR_DEBUG) $(INCLUDEDIR))


#====================================#

#----- GBNF Sources/Libs ----#

GBNF= gbnf
SOURCES_GBNF= src/gbnf.cpp src/gbnfprogram.cpp 
LIBS_GBNF= 

#--------- Test sources ---------#

SOURCES_TEST1= src/test/test1.cpp 
LIBS_TEST1= 
TEST1= $(TESTDIR)/test1

#---------  Test  list  ---------# 

TESTNAME= $(TEST1) 

#====================================#

DEBUG_INCLUDES= 
RELEASE_INCLUDES= 

BINPREFIX= 

#===================================#

all: debug

debug: debops $(GBNF) $(TESTNAME)
release: relops $(GBNF) $(TESTNAME)

debops: 
	$(eval CXXFLAGS += $(DEBUG_CXXFLAGS) $(DEBUG_INCLUDES))
	$(eval BINPREFIX = $(BINDIR_DEBUG)) 

relops: 
	$(eval CXXFLAGS += $(RELEASE_CXXFLAGS) $(RELEASE_INCLUDES)) 
	$(eval BINPREFIX = $(BINDIR_RELEASE)) 

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $*.cpp -o $*.o

$(GBNF): $(SOURCES_GBNF:.cpp=.o) $(LIBS_GBNF)
	$(CXX) -o $(BINPREFIX)/$@ $^ $(LDFLAGS)
$(GBNF)_debug: debops $(GBNF)    

#===================================#
# Tests

test_debug: debops $(TESTNAME)

$(TEST1): $(SOURCES_TEST1:.cpp=.o) $(LIBS_TEST1)
	$(CXX) -o $@ $^ $(LDFLAGS)

#===================================#

clean:
	$(RM) *.o */*.o */*/*.o */*/*/*.o

clean_all: clean
	$(RM) $(BINDIR)/*.* $(BINDIR)/*/*.* $(TESTNAME)
