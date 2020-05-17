SRC_DIR=src
BUILD_DIR=build
INCLUDE_DIR=include
BIN_DIR=bin
TEST_SRC_DIR=test
LZMA_DIR=lzma
LZMA_INCLUDE_DIR=$(LZMA_DIR)\include
LZMA_SRC_DIR=$(LZMA_DIR)\src
CRT_DIR=crt

LZMA_CS=
LZMA_CS=$(LZMA_CS) $(LZMA_SRC_DIR)\7zAlloc.c
LZMA_CS=$(LZMA_CS) $(LZMA_SRC_DIR)\Alloc.c
LZMA_CS=$(LZMA_CS) $(LZMA_SRC_DIR)\LzFind.c
LZMA_CS=$(LZMA_CS) $(LZMA_SRC_DIR)\LzFindMt.c
LZMA_CS=$(LZMA_CS) $(LZMA_SRC_DIR)\LzmaDec.c
LZMA_CS=$(LZMA_CS) $(LZMA_SRC_DIR)\LzmaEnc.c
LZMA_CS=$(LZMA_CS) $(LZMA_SRC_DIR)\LzmaLib.c
LZMA_CS=$(LZMA_CS) $(LZMA_SRC_DIR)\Threads.c
LZMA_CCS=$(LZMA_SRC_DIR)\Threads_x.cc
LZMA_OBJS=
LZMA_OBJS=$(LZMA_OBJS) $(BUILD_DIR)\7zAlloc.obj
LZMA_OBJS=$(LZMA_OBJS) $(BUILD_DIR)\Alloc.obj
LZMA_OBJS=$(LZMA_OBJS) $(BUILD_DIR)\LzFind.obj
LZMA_OBJS=$(LZMA_OBJS) $(BUILD_DIR)\LzFindMt.obj
LZMA_OBJS=$(LZMA_OBJS) $(BUILD_DIR)\LzmaDec.obj
LZMA_OBJS=$(LZMA_OBJS) $(BUILD_DIR)\LzmaEnc.obj
LZMA_OBJS=$(LZMA_OBJS) $(BUILD_DIR)\LzmaLib.obj
LZMA_OBJS=$(LZMA_OBJS) $(BUILD_DIR)\Threads.obj
LZMA_OBJS=$(LZMA_OBJS) $(BUILD_DIR)\Threads_x.obj
LZMA_HS=
LZMA_HS=$(LZMA_HS) $(LZMA_SRC_DIR)\7zAlloc.h
LZMA_HS=$(LZMA_HS) $(LZMA_SRC_DIR)\Alloc.h
LZMA_HS=$(LZMA_HS) $(LZMA_SRC_DIR)\Compiler.h
LZMA_HS=$(LZMA_HS) $(LZMA_SRC_DIR)\LzFind.h
LZMA_HS=$(LZMA_HS) $(LZMA_SRC_DIR)\LzFindMt.h
LZMA_HS=$(LZMA_HS) $(LZMA_SRC_DIR)\LzHash.h
LZMA_HS=$(LZMA_HS) $(LZMA_SRC_DIR)\LzmaDec.h
LZMA_HS=$(LZMA_HS) $(LZMA_SRC_DIR)\LzmaEnc.h
LZMA_HS=$(LZMA_HS) $(LZMA_SRC_DIR)\Precomp.h
LZMA_HS=$(LZMA_HS) $(LZMA_SRC_DIR)\Threads.h
LZMA_HS=$(LZMA_HS) $(LZMA_INCLUDE_DIR)\7zTypes.h
LZMA_HS=$(LZMA_HS) $(LZMA_INCLUDE_DIR)\LzmaLib.h

DEBUG=
!ifdef DBG
DEBUG=/DDEBUG_OUT
!endif

CXXFLAGS=/nologo /std:c++latest $(DEBUG) /Oi- /O1 /GS- /Gy- /GR- /c /I$(INCLUDE_DIR) /I$(LZMA_INCLUDE_DIR)
CXXFLAGS2=/nologo /EHsc /std:c++latest /c /I$(LZMA_INCLUDE_DIR)
CFLAGS=/nologo /Oi- /GS- /Gy- /GR- /c /I$(INCLUDE_DIR) /D_NO_CRT_STDIO_INLINE /I$(LZMA_INCLUDE_DIR)
LDFLAGS=/nologo /nodefaultlib /incremental:no /subsystem:console,5.01 /entry:Main /safeseh:no /EMITPOGOPHASEINFO /merge:.rdata=.text /align:16
LDFLAGS2=/nologo
LIBFLAGS=/nologo /nodefaultlib /subsystem:windows,5.01

TO_PACK=$(BUILD_DIR)\test.exe
BIN_PACK=$(BIN_DIR)\test.exe

all: $(BIN_DIR)\test.exe

pack: $(TO_PACK) $(BUILD_DIR)\pack.lib $(BUILD_DIR)\compress.exe Makefile
	@echo TO_PACK: $(TO_PACK)
	@echo BIN_PACK: $(BIN_PACK)
	@$(BUILD_DIR)\compress.exe $(BUILD_DIR)\test.c $(TO_PACK)
	@cl $(CXXFLAGS) $(BUILD_DIR)\test.c /Fo$(BUILD_DIR)\test.obj
	@link $(LDFLAGS) /OUT:$(BIN_PACK) $(BUILD_DIR)\test.obj $(BUILD_DIR)\pack.lib

$(BIN_DIR)\test.exe: $(BUILD_DIR)\test.exe $(BUILD_DIR)\pack.lib $(BUILD_DIR)\compress.exe Makefile
	@nmake pack

$(BUILD_DIR)\test.exe: $(TEST_SRC_DIR)\test.cpp $(TEST_SRC_DIR)\testdll.cpp $(TEST_SRC_DIR)\testdll2.cpp $(BUILD_DIR)\pack.lib $(BUILD_DIR)\compress.exe Makefile
	@cl $(CXXFLAGS2) /Fo$(BUILD_DIR)\testdll.obj $(TEST_SRC_DIR)\testdll.cpp
	@link $(LDFLAGS2) /dll /OUT:$(BUILD_DIR)\testdll.dll /implib:$(BUILD_DIR)\testdll.lib $(BUILD_DIR)\testdll.obj
	@cl $(CXXFLAGS2) /Fo$(BUILD_DIR)\testdll2.obj $(TEST_SRC_DIR)\testdll2.cpp
	@link $(LDFLAGS2) /dll /OUT:$(BUILD_DIR)\testdll2.dll /implib:$(BUILD_DIR)\testdll2.lib $(BUILD_DIR)\testdll2.obj
	@cl $(CXXFLAGS2) /Fo$(BUILD_DIR)\test.obj $(TEST_SRC_DIR)\test.cpp
	@link $(LDFLAGS2) /OUT:$(BUILD_DIR)\test.exe $(BUILD_DIR)\test.obj $(BUILD_DIR)\testdll.lib $(BUILD_DIR)\testdll2.lib

$(BUILD_DIR)\compress.exe: $(SRC_DIR)\compress.cpp $(BUILD_DIR)\LzmaLib.lib Makefile
	@cl $(CXXFLAGS2) /D_LZMA_NO_SYSTEM_SIZE_T /Fo$(BUILD_DIR)\compress.obj $(SRC_DIR)\compress.cpp
	@link $(LDFLAGS2) /OUT:$(BUILD_DIR)\compress.exe $(BUILD_DIR)\compress.obj $(BUILD_DIR)\LzmaLib.lib imagehlp.lib shlwapi.lib

$(BUILD_DIR)\pack.lib: $(BUILD_DIR)\loader.obj $(BUILD_DIR)\memoryModule.obj $(BUILD_DIR)\LzmaLib.lib $(BUILD_DIR)\ntdll.lib $(BUILD_DIR)\msvcrt.lib Makefile
	@lib $(LIBFLAGS) /OUT:$(BUILD_DIR)\pack.lib $(BUILD_DIR)\loader.obj $(BUILD_DIR)\memoryModule.obj $(BUILD_DIR)\LzmaLib.lib $(BUILD_DIR)\ntdll.lib $(BUILD_DIR)\msvcrt.lib kernel32.lib user32.lib
$(BUILD_DIR)\loader.obj: $(SRC_DIR)\loader.cpp $(INCLUDE_DIR)\memoryModule.h Makefile
	@cl $(CXXFLAGS) /D_LZMA_NO_SYSTEM_SIZE_T /Fo$(BUILD_DIR)\loader.obj $(SRC_DIR)\loader.cpp
$(BUILD_DIR)\memoryModule.obj: $(SRC_DIR)\memoryModule.cpp $(INCLUDE_DIR)\memoryModule.h Makefile
	@cl $(CXXFLAGS) /Fo$(BUILD_DIR)\memoryModule.obj $(SRC_DIR)\memoryModule.cpp

$(BUILD_DIR)\ntdll.lib: $(CRT_DIR)\ntdll.def Makefile
	@lib $(LIBFLAGS) /out:$(BUILD_DIR)\ntdll.lib /def:$(CRT_DIR)\ntdll.def
$(BUILD_DIR)\msvcrt.lib: $(CRT_DIR)\msvcrt.def Makefile
	@lib $(LIBFLAGS) /out:$(BUILD_DIR)\msvcrt.lib /def:$(CRT_DIR)\msvcrt.def

$(BUILD_DIR)\LzmaLib.lib:$(LZMA_CS) $(LZMA_CCS) $(LZMA_HS) Makefile
	@for %i in ($(LZMA_SRC_DIR)\*.c) do @cl $(CFLAGS) /O1 /Fo$(BUILD_DIR)\ %i
	@for %i in ($(LZMA_SRC_DIR)\*.cc) do @cl $(CFLAGS) /Fo$(BUILD_DIR)\ %i
	@lib $(LIBFLAGS) /out:$(BUILD_DIR)\LzmaLib.lib $(LZMA_OBJS)


.PHONY: all clean test pack

clean:
	@del /s /q $(BUILD_DIR)\*

dist-clean:
	@del /s /q $(BUILD_DIR)\*
	@del /s /q $(BIN_DIR)\*
