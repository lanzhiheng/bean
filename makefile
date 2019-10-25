CC = gcc
TARGET = bean
CFLAGS = -g -Wall
CFILES = $(wildcard *.c)

$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(CFILES) $(CFLAGS)

clean:
	-$(RM) $(TARGET)
