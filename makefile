CC = gcc -v
TARGET = bean
CFLAGS = -g -Wall
TFLAGS = -g -Wall -DTEST
CFILES = $(wildcard *.c)
LIBS = -L/usr/local/Cellar/curl/7.69.0/lib -lcurl -lldap -lz -I/usr/local/Cellar/curl/7.69.0/include

$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(CFILES) $(CFLAGS) $(LIBS)

test:
	$(CC) -o $(TARGET) $(CFILES) $(TFLAGS) $(LIBS)
	./$(TARGET)

clean:
	-$(RM) $(TARGET)
