.SUFFIXES: .cpp .cxx .h

# compiler names:
CXX		= g++ 
CC		= gcc

# flags for C++ compiler:
CFLAGS		= -I/opt/local/include -g -Wno-deprecated  
CXXFLAGS	= -std=c++11 -I/opt/local/include -g -Wno-deprecated  

# libraries to link with:

LIBPATH = -L/opt/local/lib

LDFLAGS = -lGLEW -lGL -lGLU -lglut -lm
# LDFLAGS = -framework OpenGL -framework GLUT -lglew 

OBJFILES = initShaders.o

# compile rules
.c.o:	$*.h
	@echo "Compiling C code ...."
	$(CXX) -o $*.o -c $(CXXFLAGS) $(DEFFLAGS) $(INCFLAGS) $*.c

.cpp.o:	$*.h
	@echo "Compiling C++ code ...."
	$(CXX) -o $*.o -c $(CXXFLAGS) $(DEFFLAGS) $(INCFLAGS) $*.cpp

# ***********************************************************************************
all:	curve_revolution

# soglut:	soglut.o
# 	@echo "Linking ...."
# 	$(CXX)  $(CXXFLAG) $(INCFLAGS) $(LIBPATH) $^ $(LDFLAGS) -o $@
	
curve_revolution:	initShaders.o curve_revolution.o
	@echo "Linking ...."
	$(CXX)  $(CXXFLAG) $(INCFLAGS) $(LIBPATH) $^ $(LDFLAGS) -o $@ 
		
# drawCube:	initShaders.o drawCube.o
# 	@echo "Linking ...."
# 	$(CXX)  $(CXXFLAG) $(INCFLAGS) $(LIBPATH) $^ $(LDFLAGS) -o $@ 
		
clean:	
	@echo "Clearing ..."
	rm -f *.o core *.*~ *~ soglut curve_revolution drawCube
