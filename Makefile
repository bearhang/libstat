CC = gcc

SOURCE_PATH = ./source
COMMON_PATH = ./common
TEST_PATH = ./test
BIN_PATH = ./bin

CFILES = $(wildcard $(SOURCE_PATH)/*.c)
CFILES += $(wildcard $(COMMON_PATH)/*.c)
CFILES += $(wildcard $(TEST_PATH)/*.c)

CHEADS = $(wildcard $(SOURCE_PATH)/*.h)
CHEADS += $(wildcard $(COMMON_PATH)/*.h)
CHEADS += $(wildcard $(TEST_PATH)/*.h)

OBJECTS = $(CFILES:%.c=%.o)

CCFLAGS = -g
CCFLAGS += $(sort $(addprefix -I, $(dir $(CHEADS))))

CCLINKS = -lpthread 

stub : $(OBJECTS)
	$(CC) -o $(BIN_PATH)/$@ $(OBJECTS) $(CCLINKS)
	
$(OBJECTS) : %.o : %.c $(CHEADS)
	$(CC) -c $< -o $@ $(CCFLAGS)
	
.PHONY : clean
clean :
	rm $(BIN_PATH)/stub $(OBJECTS)
