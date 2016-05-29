CC = gcc

OBJECTS = statistics.o stub.o
INCLUDES = statistics.h common/dlist.h

CCFLAGS = -g -Icommon/
CCLIBRY = -lpthread 

stub : $(OBJECTS)
	$(CC) -o stub $(OBJECTS) $(CCLIBRY)
	
stub.o : stub.c $(INCLUDES)
	$(CC) -c stub.c $(CCFLAGS) $(CCLIBRY)
	
statistics.o : statistics.c $(INCLUDES)
	$(CC) -c statistics.c $(CCFLAGS) $(CCLIBRY)
	
.PHONY : clean
clean :
	rm stub $(OBJECTS)