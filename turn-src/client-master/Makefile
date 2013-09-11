 CC = gcc
 CFLAGS = -lpthread 
 CFLAGS += -I/home/zj/nattest/turn-src/include/
 OBJS = main.o
 TARGET = natclient

$(TARGET): $(OBJS) 
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS)
main.o: main.c
	$(CC) -c $^ $(CFLAGS)

.PHONY: clean
clean:
	rm $(OBJS) $(TARGET)
