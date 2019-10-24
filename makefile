CC = gcc
TARGET = bean
CFLAGS = -g
CFILES = $(wildcard *.c)

$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(CFILES) $(CFLAGS)

clean:
	-$(RM) $(TARGET)
