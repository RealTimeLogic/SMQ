


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

LIBNAME=$(LIBPFX)SimpleMQ$(LIBEXT) 

CFLAGS+=$(IFT)src $(IFT)examples
VPATH=src:examples

# Implicit rules for making .o files from .c files
$(ODIR)/%$(O) : %.c
	$(CC) $(CFLAGS) $(OFT)$@ $<

SOURCE = selib.c SMQClient.c


examples: $(ODIR) LED-SMQ$(EXT)

$(ODIR):
	mkdir $(ODIR)

LED-SMQ$(EXT): $(ODIR)/LED-SMQ$(O) $(LIBNAME)
	$(CC) $(LNKOFT)$@ $< -L. -lSimpleMQ $(EXTRALIBS)

$(LIBNAME):  $(SOURCE:%.c=$(ODIR)/%$(O))
	$(AR) $(ARFLAGS) $(AROFT)$@ $^
	$(RANLIB) $@
