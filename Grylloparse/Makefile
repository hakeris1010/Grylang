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
# Set Dependencies: Gryltools and GRYLLOPARSE. 

INCLUDES= -I../gryltools/libGrylTools/include -I../gbnf/include
LIBCLUDES= -L../gryltools/libGrylTools/lib/static -L../gbnf/lib/static

#====================================#
#set directories
ZSH_RESULT:=$(shell mkdir -p $(TESTDIR) $(LIBDIR) \
   			$(LIBDIR)/static $(LIBDIR)/shared \
			$(BINDIR_RELEASE) $(BINDIR_DEBUG) $(INCLUDEDIR))

#--------- GRYLLOPARSE Sources ---------#

GRYLLOPARSE:=grylloparse
	   
SOURCES_GRYLLOPARSE= src/lexer.cpp \
					 src/reglex.cpp

HEADERS_GRYLLOPARSE= src/grylloparse.hpp \
			   		 src/lexer.hpp \
					 src/reglex.hpp

LIBS_GRYLLOPARSE:= -lgbnf -lgryltools

#--------- GRYLLOPARSE Program ---------#

GRYLLOPARSEPROGRAM= gparseprogram

GRYLLOPARSEPROGRAM_SOURCES= src/gparseprogram.cpp 

GRYLLOPARSEPROGRAM_LIBS:= -l$(GRYLLOPARSE) $(LIBS_GRYLLOPARSE)

#--------- Test sources ---------#

TEST_SOURCES= src/test/test1.cpp 

TEST_LIBS= -l$(GRYLLOPARSE)

#====================================#

GRYLLOPARSE_INCL= $(INCLUDEDIR)/$(GRYLLOPARSE)
GRYLLOPARSE_LIB= $(LIBDIR)/static/lib$(GRYLLOPARSE)
GRYLLOPARSE_SHARED= $(LIBDIR)/shared/lib$(GRYLLOPARSE)

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
	#LIBS_GRYLLOPARSE += -lWs2_32 -lkernel32

	GRYLLOPARSE_SHARED:= $(GRYLLOPARSE_SHARED).dll
	GRYLLOPARSE_LIB:= $(GRYLLOPARSE_LIB).a

else
    CFLAGS += -std=gnu99 -pthread	
	CXXFLAGS += -std=gnu11 -pthread
    #LDFLAGS += -pthread

	GRYLLOPARSE_SHARED:= $(GRYLLOPARSE_SHARED).so
	GRYLLOPARSE_LIB:= $(GRYLLOPARSE_LIB).a
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

debug: debops $(GRYLLOPARSE) test_main 
release: relops $(GRYLLOPARSE) test_main 

$(GRYLLOPARSE): gryllo_incl $(GRYLLOPARSE_LIB) $(GRYLLOPARSEPROGRAM) # $(GRYLLOPARSE_SHARED)   
$(GRYLLOPARSE)_debug: debops $(GRYLLOPARSE)

.cpp.o:
	$(CXX) $(CXXFLAGS) -fpic -c $*.cpp -o $*.o

gryllo_incl: $(HEADERS_GRYLLOPARSE) 
	mkdir -p $(GRYLLOPARSE_INCL) 
	for file in $^ ; do \
		cp -u $$file $(GRYLLOPARSE_INCL) ; \
	done
	
$(GRYLLOPARSE_LIB): $(SOURCES_GRYLLOPARSE:.cpp=.o) 
	$(AR) -rvsc $@ $^ 

$(GRYLLOPARSE_SHARED): $(SOURCES_GRYLLOPARSE:.cpp=.o) 
	$(CXX) -shared -o $(GRYLLOPARSE_SHARED) $^ -Wl,--whole-archive $(LIBS_GRYLLOPARSE) -Wl,--no-whole-archive $(LDFLAGS) 

$(GRYLLOPARSEPROGRAM): $(GRYLLOPARSEPROGRAM_SOURCES:.cpp=.o)
	$(CXX) $(LDFLAGS) -o $(BINPREFIX)/$@ $^ $(GRYLLOPARSEPROGRAM_LIBS)

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
	$(RM) *.o */*.o */*/*.o 

clean_all: clean
	$(RM) -rf bin lib

#nyaaaaa "$(RM) -rf $(BINDIR) $(LIBDIR)"

