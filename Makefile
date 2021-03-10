EXEC_FILE := outfit
CC := gcc
CFLAGS := -g -Wall
LIBS := -lsqlite3 -lcurl

$(EXEC_FILE) : main.o maintenance.o display.o weather.o 
	$(CC) $(CFLAGS) -o $(EXEC_FILE) main.o maintenance.o \
	display.o weather.o $(LIBS)

main.o : main.c display.h weather.h maintenance.h
	$(CC) $(CFLAGS) -c main.c

display.o : display.c display.h weather.h
	$(CC) $(CFLAGS) -c display.c 

weather.o : weather.c weather.h
	$(CC) $(CFLAGS) -c weather.c

maintenance.o : maintenance.c maintenance.h
	$(CC) $(CFLAGS) -c maintenance.c

clean :
	rm -f *.o $(EXEC_FILE)
