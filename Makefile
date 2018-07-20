
TARGET	=	deparam
OBJS	=	deparam.o 

.c.o:
	gcc -Wall -c -g $< -o $@

$(TARGET): $(OBJS)
	gcc -Wall -Wextra -o $(TARGET) $(OBJS)

deparam.o: deparam.c

clean:
	-rm $(TARGET) $(OBJS)