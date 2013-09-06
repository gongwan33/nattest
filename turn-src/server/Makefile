 CC = gcc
 CFLAGS = -lpthread 
 OBJS = main.o
 TARGET = natserver

$(TARGET): $(OBJS) 
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS)
main.o: main.c
	$(CC) -c $^ $(CFLAGS)

.PHONY: clean
clean:
	rm $(OBJS) $(TARGET)
