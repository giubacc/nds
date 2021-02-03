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

CCFLAGS = -Wall -g
CCC = g++
LDLIBS= -lpthread

.cpp.o:
	$(CCC) $(INCLUDES) $(CCFLAGS) -c $< -o $@

default: ndg
	
all: clean ndg
	
ndg: $(OBJ)
	$(CCC) -o $(OUTDIR)/ndg $(OBJ) $(LDLIBS)

clean:
	rm -f $(OBJ) $(OUTDIR)/ndg
