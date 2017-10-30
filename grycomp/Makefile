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

#- GrylTools Libs/Includes --#

GRYLTOOLS_INCLUDE= -I../gryltools/include
GRYLTOOLS_LIBCLUDE= -L../gryltools/lib

#----- GRYCOMP Sources/Libs ----#

GRYCOMP= gbnf
SOURCES_GRYCOMP= src/main.cpp src/parser.cpp src/lexer.cpp 
LIBS_GRYCOMP= -lgryltools

#--------- Test sources ---------#

SOURCES_TEST1= src/test/test1.cpp 
LIBS_TEST1= -lgryltools
TEST1= $(TESTDIR)/test1

#---------  Test  list  ---------# 

TESTNAME= $(TEST1) 

#====================================#

DEBUG_INCLUDES= 
RELEASE_INCLUDES= 

BINPREFIX= 

# Set the libraries

LDFLAGS += $(GRYLTOOLS_LIBCLUDE)
CXXFLAGS += $(GRYLTOOLS_INCLUDE)

#===================================#

all: debug

debug: debops $(GRYCOMP) $(TESTNAME)
release: relops $(GRYCOMP) $(TESTNAME)

debops: 
	$(eval CXXFLAGS += $(DEBUG_CXXFLAGS) $(DEBUG_INCLUDES))
	$(eval BINPREFIX = $(BINDIR_DEBUG)) 

relops: 
	$(eval CXXFLAGS += $(RELEASE_CXXFLAGS) $(RELEASE_INCLUDES)) 
	$(eval BINPREFIX = $(BINDIR_RELEASE)) 

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $*.cpp -o $*.o

$(GRYCOMP): $(SOURCES_GRYCOMP:.cpp=.o) 
	$(CXX) $(LDFLAGS) -o $(BINPREFIX)/$@ $^ $(LIBS_GRYCOMP) 
$(GRYCOMP)_debug: debops $(GRYCOMP)    

#===================================#
# Tests

test_debug: debops $(TESTNAME)

$(TEST1): $(SOURCES_TEST1:.cpp=.o) 
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS_TEST1)

#===================================#

clean:
	$(RM) *.o */*.o */*/*.o */*/*/*.o

clean_all: clean
	$(RM) $(BINDIR)/*.* $(BINDIR)/*/*.* $(TESTNAME)
