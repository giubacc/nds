SRC = 	./src/main.cpp\
		./src/bbuf.cpp\
		./src/selector.cpp\
		./src/connection.cpp\
		./src/peer.cpp\
		./jsoncpp/src/lib_json/json_reader.cpp\
		./jsoncpp/src/lib_json/json_value.cpp\
		./jsoncpp/src/lib_json/json_writer.cpp
		
OBJ = $(SRC:.cpp=.o)
OUTDIR = .
INCLUDES = 	-I./src\
			-I./jsoncpp/include\
			-I./spdlog/include\
			-I./clipp/include\

CCFLAGS = -Wall -g
CCC = g++
LDLIBS= -lpthread

.cpp.o:
	$(CCC) $(INCLUDES) $(CCFLAGS) -c $< -o $@

default: nds
	
all: clean nds
	
nds: $(OBJ)
	$(CCC) -o $(OUTDIR)/nds $(OBJ) $(LDLIBS)

clean:
	rm -f $(OBJ) $(OUTDIR)/nds
