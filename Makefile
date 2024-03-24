


#General macros
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)
#export AR
ifndef RANLIB
export RANLIB := ranlib
#export ARFLAGS
export AROFT := $(SPACE)
export CC := gcc
export CXX := g++
endif
export O := .o
export IFT := -I
export OFT := -o$(SPACE)
export LNKOFT := -o$(SPACE)
export LIBPFX := lib
export LIBEXT := .a
CFLAGS+=$(XCFLAGS)
CFLAGS+=-DB_LITTLE_ENDIAN
CFLAGS+=-Wall -c
ifeq (debug,$(build))
CFLAGS += -g
else
CFLAGS += -Os -O3
endif

CFLAGS += -DXPRINTF

ifndef PLAT
PLAT=Posix
CFLAGS+=$(IFT)src/arch/Posix
EXTRALIBS += -lrt
endif

ifndef ODIR
ODIR = obj
endif

LIBNAME=$(LIBPFX)ExampleLib$(LIBEXT) 

CFLAGS+=$(IFT)src $(IFT)examples
VPATH=src:examples

# Implicit rules for making .o files from .c files
$(ODIR)/%$(O) : %.c
	$(CC) $(CFLAGS) $(OFT)$@ $<
# Implicit rules for making .o files from .cpp files
$(ODIR)/%$(O) : %.cpp
	$(CXX) $(CFLAGS) $(OFT)$@ $<

SOURCE = selib.c SMQClient.c

.PHONY : examples clean

CXX_AVAILABLE := $(shell command -v g++x)

# Conditional compilation based on g++ availability
ifeq ($(CXX_AVAILABLE),)

$(info g++ not found, assuming gcc is installed. Compiling the LED-SMQ example.)
examples: LED-SMQ$(EXT)

else
# g++ available, proceed as normal

ifneq ($(wildcard ../JSON/.*),)
examples: $(ODIR) publish$(EXT) subscribe$(EXT) bulb$(EXT) LED-SMQ$(EXT)
VPATH += ../JSON/src
SOURCE += AllocatorIntf.c\
	  BaAtoi.c\
	  BufPrint.c\
	  JDecoder.c\
	  JEncoder.c\
	  JParser.c\
	  JVal.c
CFLAGS += -I../JSON/inc -DUSE_JSON=1 -fno-exceptions
publish$(EXT): $(ODIR)/publish$(O) $(LIBNAME)
	$(CC) $(LNKOFT)$@ $< -L. -lExampleLib $(EXTRALIBS)
subscribe$(EXT): $(ODIR)/subscribe$(O) $(LIBNAME)
	$(CC) $(LNKOFT)$@ $< -L. -lExampleLib $(EXTRALIBS)
else
examples: $(ODIR) bulb$(EXT) LED-SMQ$(EXT)
$(info No JSON directory. Excluding the examples 'publish' and 'subscribe'.)
endif

endif

$(ODIR):
	mkdir $(ODIR)

LED-SMQ$(EXT): $(ODIR)/LED-SMQ$(O) $(LIBNAME)
	$(CC) $(LNKOFT)$@ $< -L. -lExampleLib $(EXTRALIBS)

bulb$(EXT): $(ODIR)/bulb$(O) $(LIBNAME)
	$(CC) $(LNKOFT)$@ $< -L. -lExampleLib $(EXTRALIBS)

$(LIBNAME):  $(SOURCE:%.c=$(ODIR)/%$(O))
	$(AR) $(ARFLAGS) $(AROFT)$@ $^
	$(RANLIB) $@

clean:
	rm -rf $(ODIR) $(LIBNAME) LED-SMQ$(EXT) bulb$(EXT) publish$(EXT) subscribe$(EXT)

