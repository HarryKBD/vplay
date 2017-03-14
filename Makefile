# the compiler: gcc for C program, define as g++ for C++
CC = gcc
# compiler flags:
#CFLAGS  = -g -Wall
CFLAGS  =
#LFLAGS = 
LIBS = -lavutil -lavformat -lavcodec -lavfilter -lavutil -lswresample -lm -lpthread -lswscale -ldl -lSDL

# the build target executable:
TARGET = vplay
SRCS = buf.c vplay.c sdl.c

all: $(TARGET)

OBJS =  $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(OBJS) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) $(TARGET) $(OBJS)
