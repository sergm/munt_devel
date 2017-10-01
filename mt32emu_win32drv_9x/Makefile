!if "$(DEBUG)" == "NO"
CFLAGS  =
LFLAGS  = 
!else
CFLAGS  =-Zi -DDEBUG
LFLAGS  =/CO
!endif

INCLUDES=
LIBS=link /ALIGN:16/ONERROR:NOEXE/MAP/NODEFAULTLIBRARYSEARCH

CC      =cl -c -nologo -W3 -Alnw -Oxws -Zp -Gs $(CFLAGS) $(INCLUDES)
LINK    =link $(LFLAGS) /ALIGN:16/ONERROR:NOEXE/MAP/NODEFAULTLIBRARYSEARCH

OBJ=winmm_drv.obj
LIBS=mdllcew libw mmsystem toolhelp
DEF=winmm_drv.def
TARGET=mt32emu.drv

all: $(TARGET)

$(OBJ): winmm_drv.c
	$(CC) -NT _FIX $*.c

$(TARGET): $(OBJ) $(DEF)
        $(LINK) $(OBJ),$(TARGET),,$(LIBS),$(DEF)

clean:
	@del *.obj 2> NUL
	@del *.map 2> NUL
	@del *.pdb 2> NUL
	@del *.drv 2> NUL
