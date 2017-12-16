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

#--------- Directories ---------#

LIBDIR= lib
INCLUDEDIR= include
BINDIR= bin
TESTDIR= $(BINDIR)/test

BINDIR_RELEASE= $(BINDIR)/release
BINDIR_DEBUG= $(BINDIR)/debug

#====================================#
# Set Dependencies: Gryltools and YUKIPARSE. 

INCLUDES= -I../gryltools/libGrylTools/include -I../gbnf/include
LIBCLUDES= -L../gryltools/libGrylTools/lib/static -L../gbnf/lib/static

#====================================#
#set directories
ZSH_RESULT:=$(shell mkdir -p $(TESTDIR) $(LIBDIR) \
   			$(LIBDIR)/static $(LIBDIR)/shared \
			$(BINDIR_RELEASE) $(BINDIR_DEBUG) $(INCLUDEDIR))

#--------- YUKIPARSE Sources ---------#

YUKIPARSE:=yukiparse
	   
SOURCES_YUKIPARSE= 

HEADERS_YUKIPARSE= src/yukiparse.hpp

LIBS_YUKIPARSE:= -lgbnf -lgryltools

#--------- YUKIPARSE Program ---------#

YUKIPARSEPROGRAM= yukiprogram

YUKIPARSEPROGRAM_SOURCES= src/yukiprogram.cpp 

YUKIPARSEPROGRAM_LIBS:= -l$(YUKIPARSE) $(LIBS_YUKIPARSE)

#--------- Test sources ---------#

TEST_SOURCES= src/test/test1.cpp 

TEST_LIBS= -l$(YUKIPARSE)

#====================================#

YUKIPARSE_INCL= $(INCLUDEDIR)/$(YUKIPARSE)
YUKIPARSE_LIB= $(LIBDIR)/static/lib$(YUKIPARSE)
YUKIPARSE_SHARED= $(LIBDIR)/shared/lib$(YUKIPARSE)

YUKIPARSE= gbnf_whole_franchise

#====================================#
# Includes/Libcludes.

DEBUG_INCLUDES= -Isrc/
DEBUG_LIBCLUDES= -L$(LIBDIR)/static

RELEASE_INCLUDES= -I$(INCLUDEDIR)
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
	#LIBS_YUKIPARSE += -lWs2_32 -lkernel32

	YUKIPARSE_SHARED:= $(YUKIPARSE_SHARED).dll
	YUKIPARSE_LIB:= $(YUKIPARSE_LIB).a

else
    CFLAGS += -std=gnu99 -pthread	
	CXXFLAGS += -std=gnu11 -pthread
    #LDFLAGS += -pthread

	YUKIPARSE_SHARED:= $(YUKIPARSE_SHARED).so
	YUKIPARSE_LIB:= $(YUKIPARSE_LIB).a
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

debug: debops $(YUKIPARSE) test_main 
release: relops $(YUKIPARSE) test_main 

$(YUKIPARSE): yuki_incl $(YUKIPARSE_LIB) $(YUKIPARSEPROGRAM) # $(YUKIPARSE_SHARED)   
$(YUKIPARSE)_debug: debops $(YUKIPARSE)

.cpp.o:
	$(CXX) $(CXXFLAGS) -fpic -c $*.cpp -o $*.o

yuki_incl: $(HEADERS_YUKIPARSE) 
	mkdir -p $(YUKIPARSE_INCL) 
	for file in $^ ; do \
		cp -u $$file $(YUKIPARSE_INCL) ; \
	done
	
$(YUKIPARSE_LIB): $(SOURCES_YUKIPARSE:.cpp=.o) 
	$(AR) -rvsc $@ $^ 

$(YUKIPARSE_SHARED): $(SOURCES_YUKIPARSE:.cpp=.o) 
	$(CXX) -shared -o $(YUKIPARSE_SHARED) $^ -Wl,--whole-archive $(LIBS_YUKIPARSE) -Wl,--no-whole-archive $(LDFLAGS) 

$(YUKIPARSEPROGRAM): $(YUKIPARSEPROGRAM_SOURCES:.cpp=.o)
	$(CXX) $(LDFLAGS) -o $(BINPREFIX)/$@ $^ $(YUKIPARSEPROGRAM_LIBS)

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
	

#nyaaaaa "$(RM) -rf $(BINDIR) $(LIBDIR)"

