TARGET = pmfPlayer

OBJS = src/decoder.o src/reader.o src/video.o src/audio.o src/pmfplayer.o src/main3.o src/cppatch.o

BUILD_PRX = 1

INCDIR = ./include
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR = ./lib
LDFLAGS = 
LIBS = -lstdc++  -lintraFont -lpspsystemctrl_user -lpspumd -lpspgu -lpspmpeg -lpspmpegbase -lpspaudio -lpsppower
EXTRA_TARGETS=EBOOT.PBP
PSP_EBOOT_TITLE=Pmf Viewer 0.1
PSP_EBOOT_ICON=ICON0.PNG

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
