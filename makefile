SRC = 	./src/main.cpp\
		./src/bbuf.cpp\
		./src/selector.cpp\
		./src/connection.cpp\
		./src/peer.cpp
		
OBJ = $(SRC:.cpp=.o)
OUTDIR = .
INCLUDES = 	-I./src\
			-I./jsoncpp/include\
			-I./spdlog/include\
			-I./clipp/include\
			-I./googletest/googletest/include

CCFLAGS = -Wall -g
CCC = g++
LDLIBS= -L./jsoncpp/build/src/lib_json -ljsoncpp -lpthread

.cpp.o:
	$(CCC) $(INCLUDES) $(CCFLAGS) -c $< -o $@

default: nds
	
all: clean nds
	
nds: $(OBJ)
	$(CCC) -o $(OUTDIR)/nds $(OBJ) $(LDLIBS)

clean:
	rm -f $(OBJ) $(OUTDIR)/nds
