CC = gcc -v
TARGET = bean
CFLAGS = -g -Wall
TFLAGS = -g -Wall -DTEST
CFILES = $(wildcard *.c)
GC = -Igc/include -lgc -Lgc/lib
CURL = -Icurl/include -lcurl -lldap -lz -Lcurl/lib

$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(CFILES) $(CFLAGS) $(LIBS) $(GC) $(CURL)

test:
	$(CC) -o $(TARGET) $(CFILES) $(TFLAGS) $(LIBS)
	./$(TARGET)

clean:
	-$(RM) $(TARGET)
