CC = gcc
CFLAGS = -lpthread $(INCLUDE) -g
OBJS = main.o  
TARGET = natserver
INCLUDE = -I../Include 

$(TARGET): $(OBJS) 
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS)

main.o: main.c 
	$(CC) -c $^ $(CFLAGS)

.PHONY: clean
clean:
	rm $(OBJS) $(TARGET)
