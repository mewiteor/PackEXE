SRC_DIR=src
BUILD_DIR=build
INCLUDE_DIR=include
BIN_DIR=bin
TEST_SRC_DIR=test


CXXFLAGS=/nologo /std:c++latest /Oi- /O1 /GS- /Gy- /GR- /c /I$(INCLUDE_DIR)
CXXFLAGS2=/nologo /EHsc /std:c++latest /c
LDFLAGS=/nologo /nodefaultlib /incremental:no /subsystem:console,5.01 /entry:Main /safeseh:no /EMITPOGOPHASEINFO /merge:.rdata=.text
LDFLAGS2=/nologo
LIBFLAGS=/nologo /nodefaultlib /subsystem:windows,5.01

all: $(BIN_DIR)\test.exe

$(BIN_DIR)\test.exe: $(TEST_SRC_DIR)\test.cpp $(BUILD_DIR)\pack.lib $(BUILD_DIR)\compress.exe Makefile
	@cl $(CXXFLAGS2) /Fo$(BUILD_DIR)\test.obj $(TEST_SRC_DIR)\test.cpp
	@link $(LDFLAGS2) /OUT:$(BUILD_DIR)\test.exe $(BUILD_DIR)\test.obj
	@$(BUILD_DIR)\compress.exe $(BUILD_DIR)\test.c $(BUILD_DIR)\test.exe
	@cl $(CXXFLAGS) $(BUILD_DIR)\test.c /Fo$(BUILD_DIR)\test.obj
	@link $(LDFLAGS) /OUT:$(BIN_DIR)\test.exe $(BUILD_DIR)\test.obj $(BUILD_DIR)\pack.lib Cabinet.lib kernel32.lib user32.lib

$(BUILD_DIR)\compress.exe: $(SRC_DIR)\compress.cpp Makefile
	@cl $(CXXFLAGS2) /Fo$(BUILD_DIR)\compress.obj $(SRC_DIR)\compress.cpp
	@link $(LDFLAGS2) /OUT:$(BUILD_DIR)\compress.exe $(BUILD_DIR)\compress.obj Cabinet.lib

$(BUILD_DIR)\pack.lib: $(BUILD_DIR)\loader.obj $(BUILD_DIR)\memoryModule.obj Makefile
	@lib $(LIBFLAGS) /OUT:$(BUILD_DIR)\pack.lib $(BUILD_DIR)\loader.obj $(BUILD_DIR)\memoryModule.obj

$(BUILD_DIR)\loader.obj: $(SRC_DIR)\loader.cpp $(INCLUDE_DIR)\memoryModule.h Makefile
	@cl $(CXXFLAGS) /Fo$(BUILD_DIR)\loader.obj $(SRC_DIR)\loader.cpp
$(BUILD_DIR)\memoryModule.obj: $(SRC_DIR)\memoryModule.cpp $(INCLUDE_DIR)\memoryModule.h Makefile
	@cl $(CXXFLAGS) /Fo$(BUILD_DIR)\memoryModule.obj $(SRC_DIR)\memoryModule.cpp

.PHONY: all clean

clean:
	@del /s /q $(BUILD_DIR)\*

dist-clean:
	@del /s /q $(BUILD_DIR)\*
	@del /s /q $(BIN_DIR)\*
