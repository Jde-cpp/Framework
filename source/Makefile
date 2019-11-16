CCACHE_EXISTS := $(shell ccache -V)
ifdef CCACHE_EXISTS
    CC := ccache $(CC)
    CXX := ccache g++-8
endif
DEBUG ?= 1
CXXFLAGS_BASE = -c -g -pthread -fPIC -std=c++17 -Wall -Wno-unknown-pragmas -Wno-ignored-attributes -DJDE_NATIVE_EXPORTS
BIN_DIR_BASE:=../../bin/
OUT_DIR_BASE := .obj/
LIB_BASE = 
#-llttng-ust -lstdc++fs -llzma -lmatio
ifeq ($(DEBUG), 1)
	OUT_DIR=$(OUT_DIR_BASE)debug
	CXXFLAGS= $(CXXFLAGS_BASE) -O0 -I$(OUT_DIR) -fsanitize=address -fno-omit-frame-pointer
	BIN_DIR=$(BIN_DIR_BASE)debug
	LIB=-lasan $(LIB_BASE)
else
	OUT_DIR=$(OUT_DIR_BASE)release
	CXXFLAGS= $(CXXFLAGS_BASE) -march=native -DNDEBUG -O3 -I$(OUT_DIR)
	BIN_DIR=$(BIN_DIR_BASE)release
	LIB=$(LIB_BASE)
endif

OUTPUT = $(BIN_DIR)/libMy.so

# test:
# 	@echo $(DEBUG)
# 	@echo $(OUTPUT)

#CXX=g++-8

OBJECTS = $(OUT_DIR)/DateTime.o $(OUT_DIR)/Diagnostics.o $(OUT_DIR)/Exception.o $(OUT_DIR)/MathUtilities.o $(OUT_DIR)/Stopwatch.o $(OUT_DIR)/StringUtilities.o $(OUT_DIR)/Cache.o $(OUT_DIR)/Settings.o\
	$(OUT_DIR)/Application.o $(OUT_DIR)/ApplicationLinux.o\
	$(OUT_DIR)/Logging.o $(OUT_DIR)/ServerSink.o $(OUT_DIR)/messages.pb.o\
	$(OUT_DIR)/Socket.o $(OUT_DIR)/ProtoClient.o $(OUT_DIR)/ProtoServer.o\
	$(OUT_DIR)/File.o $(OUT_DIR)/DiskWatcher.o\
	$(OUT_DIR)/Thread.o $(OUT_DIR)/Interrupt.o $(OUT_DIR)/InterruptibleThread.o $(OUT_DIR)/Pool.o\
	$(OUT_DIR)/DBException.o $(OUT_DIR)/DataType.o $(OUT_DIR)/Database.o $(OUT_DIR)/DBQueue.o
#$(OUT_DIR)/XZ.o  
LDFLAGS = -pthread -shared -Wl,-z,origin -Wl,-rpath='$$ORIGIN'

all: $(OUTPUT)
	$(NOECHO) $(NOOP)

$(OUTPUT): $(OUT_DIR)/stdafx.h.gch $(OBJECTS)
	g++-8 -L$(BIN_DIR) $(LDFLAGS) $(OBJECTS) -o$(OUTPUT) $(LIB)

$(OUT_DIR)/stdafx.h.gch: pc.h TypeDefs.h
	$(CXX) $(CXXFLAGS) ./pc.h -o $(OUT_DIR)/stdafx.h.gch -I/home/duffyj/code/libraries/spdlog/include -I$BOOST_ROOT -I/home/duffyj/code/libraries/json/include -I/home/duffyj/code/libraries/eigen
######################################################################################
$(OUT_DIR)/%.o: ./application/%.cpp application/%.h pc.h 
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@

$(OUT_DIR)/Logging.o: ./log/Logging.cpp log/Logging.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@ -I/home/duffyj/code/libraries/spdlog/include
#$(OUT_DIR)/%.o: ./log/%.cpp log/%.h pc.h
#	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@
# -I/home/duffyj/code/libraries/json/include 
$(OUT_DIR)/%.o: ./log/server/%.cpp log/server/%.h pc.h 
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@ -I/home/duffyj/code/libraries/spdlog/include 
$(OUT_DIR)/%.o: ./log/server/proto/%.cc log/server/proto/%.h  pc.h 
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@ -I/home/duffyj/code/libraries/spdlog/include -DJDE_NATIVE_VISIBILITY= 
######################################################################################
$(OUT_DIR)/%.o: ./db/%.cpp db/%.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@ 

$(OUT_DIR)/%.o: ./settings/%.cpp settings/%.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@ -I/home/duffyj/code/libraries/json/include

$(OUT_DIR)/%.o: ./io/%.cpp io/%.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@

$(OUT_DIR)/%.o: ./io/zip/%.cpp io/zip/%.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@

$(OUT_DIR)/%.o: ./io/sockets/%.cpp io/sockets/%.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@

$(OUT_DIR)/%.o: ./%.cpp %.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@ -I/home/duffyj/code/libraries/spdlog/include 
######################################################################################
$(OUT_DIR)/%.o: ./math/%.cpp ./math/%.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@ -Wno-int-in-bool-context -I/home/duffyj/code/libraries/eigen
$(OUT_DIR)/cnpy.o: ./math/io/numpy/cnpy.cpp ./math/io/numpy/cnpy.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@

$(OUT_DIR)/%.o: ./math/io/MatLab/%.cpp ./math/io/MatLab/%.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@ -I/home/duffyj/code/libraries/matio/src -I/home/duffyj/code/libraries/eigen

$(OUT_DIR)/%.o: ./threading/%.cpp ./threading/%.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@

#$(OUT_DIR)/XZ.o: Zip/XZ.cpp Zip/XZ.h pc.h
#	$(CXX) $(CXXFLAGS) $(INCLUDE) $< -o$@
#$(OUT_DIR)/LzmaDec.o: /usr/include/lzma/LzmaDec.c 7z/Zip.h pc.h
#	$(CXX) $(CXXFLAGS) $< -o$@

clean:
	rm -rf -d $(OUT_DIR)/*.*
	rm -rf -d $(OUTPUT)