CC = gcc
TARGET = bean
CFLAGS = -g -Wall -v
TFLAGS = -g -Wall -DTEST
CFILES = $(wildcard *.c)

$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(CFILES) $(CFLAGS)

test:
	$(CC) -o $(TARGET) $(CFILES) $(TFLAGS)
	./$(TARGET)

clean:
	-$(RM) $(TARGET)
