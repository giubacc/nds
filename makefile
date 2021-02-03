SRC = 	./main.cpp
		
OBJ = $(SRC:.cpp=.o)
OUTDIR = .
INCLUDES = 	-I./src
			
CCFLAGS = -Wall -g
CCC = g++
LDLIBS=

.cpp.o:
	$(CCC) $(INCLUDES) $(CCFLAGS) -c $< -o $@

default: ndg
	
all: clean ndg
	
ndg: $(OBJ)
	$(CCC) -o $(OUTDIR)/ndg $(OBJ) $(LDLIBS)

clean:
	rm -f $(OBJ) $(OUTDIR)/ndg
