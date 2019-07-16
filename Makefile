#definitions
DIR_INC = ./src
DIR_SRC = ./src
DIR_OBJ = ./obj
SOURCE  := $(wildcard ${DIR_SRC}/*.cc)
OBJS    := $(patsubst ${DIR_SRC}/%.cc,${DIR_OBJ}/%.o, $(SOURCE))
TARGET  := contest.out

#compiling parameters
CC      := g++
LIBS    :=
LDFLAGS :=
DEFINES := $(FLAG)
INCLUDE := -I ${DIR_INC}
CFLAGS  := -g -Wall -O3 -std=c++11 $(DEFINES) $(INCLUDE)
CXXFLAGS:= $(CFLAGS)

#commands
.PHONY : all objs rebuild clean init ctags
all : init $(TARGET)

objs : init $(OBJS)

rebuild : clean all

clean :
	rm -rf $(DIR_OBJ)
	rm -f $(TARGET)
	rm -f tags

init :
	if [ ! -d obj ]; then mkdir obj; fi

ctags :
	ctags -R

$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

${DIR_OBJ}/%.o:${DIR_SRC}/%.cc
	$(CC) $(CXXFLAGS) -c $< -o $@
