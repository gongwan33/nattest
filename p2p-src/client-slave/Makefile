CC = gcc
CFLAGS = -lpthread -$(INCLUDE)
OBJS = main.o
TARGET = natslave
INCLUDE = I../Include

$(TARGET): $(OBJS) 
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS)
main.o: main.c
	$(CC) -c $^ $(CFLAGS)

.PHONY: clean
clean:
	rm $(OBJS) $(TARGET)
