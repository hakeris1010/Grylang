#mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
#current_dir := $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))

CXX = g++
AR = ar

CXXFLAGS= -std=c++14 
LDFLAGS= 

TEST_CXXFLAGS= -std=c++14 
TEST_LDFLAGS=
TEST_EXEC_ARGS=

DEBUG_CXXFLAGS= -g -Wall -O0 
DEBUG_LDFLAGS=
RELEASE_CXXFLAGS= -O2
RELEASE_LDFLAGS=

LIBDIR= lib
INCLUDEDIR= include
BINDIR= bin
TESTDIR= $(BINDIR)/test

BINDIR_RELEASE= $(BINDIR)/release
BINDIR_DEBUG= $(BINDIR)/debug

#====================================#
# Set Dependencies: GrylTools Libs/Includes 

INCLUDES= -I../gryltools/libGrylTools/include
LIBCLUDES= -L../gryltools/libGrylTools/lib/static

#====================================#
#set directories
ZSH_RESULT:=$(shell mkdir -p $(TESTDIR) $(LIBDIR) \
   			$(LIBDIR)/static $(LIBDIR)/shared \
			$(BINDIR_RELEASE) $(BINDIR_DEBUG) $(INCLUDEDIR))

#--------- GBNF Sources ---------#
       
SOURCES_GBNF= src/egbnfparser.cpp \
			  src/gbnfcodegen.cpp \
			  src/gbnfconverter.cpp 

HEADERS_GBNF= src/gbnf.hpp

LIBS_GBNF:= -lgryltools

#--------- GBNF Program ---------#

GBNFPROGRAM= gbnf

GBNFPROGRAM_SOURCES= src/gbnfprogram.cpp 

GBNFPROGRAM_LIBS:= -lgbnf $(LIBS_GBNF)

#--------- Test sources ---------#

TEST_SOURCES= src/test/test1.cpp 

TEST_LIBS= -lgbnf

#====================================#

GBNF_INCL= $(INCLUDEDIR)/gbnf/
GBNF_LIB= $(LIBDIR)/static/libgbnf
GBNF_SHARED= $(LIBDIR)/shared/libgbnf

GBNF= gbnf_whole_franchise

#====================================#
# Includes/Libcludes.

DEBUG_INCLUDES= -Isrc/
DEBUG_LIBCLUDES= -L$(LIBDIR)/static
RELEASE_INCLUDES= -I$(GBNF_INCL) 
RELEASE_LIBCLUDES= -L$(LIBDIR)/static

TEST_INCLUDES=  -I$(INCLUDEDIR)
TEST_LIBCLUDES= -L$(LIBDIR)/static

BINPREFIX= $(BINDIR_DEBUG) # (Default is debug)

#===================================#
# Add global include search directories.

CXXFLAGS += $(INCLUDES)
LDFLAGS += $(LIBCLUDES)

# set os-dependent stuff
ifeq ($(OS),Windows_NT)
    #LDFLAGS += -lkernel32 -lWs2_32
	#LIBS_GBNF += -lWs2_32 -lkernel32

	GBNF_SHARED:= $(GBNF_SHARED).dll
	GBNF_LIB:= $(GBNF_LIB).a

else
    CFLAGS += -std=gnu99 -pthread	
	CXXFLAGS += -std=gnu11 -pthread
    #LDFLAGS += -pthread

	GBNF_SHARED:= $(GBNF_SHARED).so
	GBNF_LIB:= $(GBNF_LIB).a
endif

#====================================#


all: debug 

debops: 
	$(eval CFLAGS += $(DEBUG_CFLAGS) $(DEBUG_INCLUDES))
	$(eval CXXFLAGS += $(DEBUG_CXXFLAGS) $(DEBUG_INCLUDES))
	$(eval LDFLAGS += $(DEBUG_LDFLAGS) $(DEBUG_LIBCLUDES))
	$(eval BINPREFIX = $(BINDIR_DEBUG)) 

relops: 
	$(eval CFLAGS += $(RELEASE_CFLAGS) $(RELEASE_INCLUDES)) 
	$(eval CXXFLAGS += $(RELEASE_CXXFLAGS) $(RELEASE_INCLUDES)) 
	$(eval LDFLAGS += $(RELEASE_LDFLAGS) $(RELEASE_LIBCLUDES)) 
	$(eval BINPREFIX = $(BINDIR_RELEASE)) 

debug: debops $(GBNF) test_main 
release: relops $(GBNF) test_main 

$(GBNF): gbnf_incl $(GBNF_LIB) $(GBNFPROGRAM) # $(GBNF_SHARED)   
$(GBNF)_debug: debops $(GBNF)

.cpp.o:
	$(CXX) $(CXXFLAGS) -fpic -c $*.cpp -o $*.o

gbnf_incl: $(HEADERS_GBNF) $(HEADERS_GBNFPP)
	mkdir -p $(GBNF_INCL) 
	for file in $^ ; do \
		cp -u $$file $(GBNF_INCL) ; \
	done
	
$(GBNF_LIB): $(SOURCES_GBNF:.cpp=.o) 
	$(AR) -rvsc $@ $^ 

$(GBNF_SHARED): $(SOURCES_GBNF:.cpp=.o) 
	$(CXX) -shared -o $(GBNF_SHARED) $^ -Wl,--whole-archive $(LIBS_GBNF) -Wl,--no-whole-archive $(LDFLAGS) 

$(GBNFPROGRAM): $(GBNFPROGRAM_SOURCES:.cpp=.o)
	$(CXX) $(LDFLAGS) -o $(BINPREFIX)/$@ $^ $(GBNFPROGRAM_LIBS)

#===================================#
# Tests

test_main: test_debops tests
	for file in $(TESTDIR)/* ; do \
		./$$file $(TEST_EXEC_ARGS) ; \
	done

test_debops: 
	$(eval CFLAGS   += $(TEST_CFLAGS)   $(TEST_INCLUDES) )
	$(eval CXXFLAGS += $(TEST_CXXFLAGS) $(TEST_INCLUDES) )
	$(eval LDFLAGS  += $(TEST_LDFLAGS)  $(TEST_LIBCLUDES))

tests: $(TEST_SOURCES:.cpp=.o) 
	for file in $^ ; do \
		y=$${file%.o} ; \
		exe=$(TESTDIR)/$${y##*/} ; \
		if [ ! -f $$exe ] || [ $$file -nt $$exe ] ; then \
			echo "Recompiling test $$exe ..." ; \
			$(CXX) -o $$exe $$file $(TEST_LIBS) $(LDFLAGS) ; \
		fi ; \
	done

#===================================#

clean:
	$(RM) *.o */*.o */*/*.o */*/*/*.o

clean_all: clean
	$(RM) -rf $(BINDIR) $(LIBDIR)

# $(RM) -rf $(BINDIR)/* $(BINDIR)/*/* $(BINDIR)/*/*/* $(LIBDIR)/* 

