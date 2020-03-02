CC = gcc
TARGET = bean
CFLAGS = -g -Wall
TFLAGS = -g -Wall -DTEST
CFILES = $(wildcard *.c)
LIBS = -lcurl

$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(CFILES) $(CFLAGS) $(LIBS)

test:
	$(CC) -o $(TARGET) $(CFILES) $(TFLAGS) $(LIBS)
	./$(TARGET)

clean:
	-$(RM) $(TARGET)
