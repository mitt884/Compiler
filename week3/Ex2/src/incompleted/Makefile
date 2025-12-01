CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lm

# Danh sách source và object
SRCS = main.c parser.c scanner.c reader.c charcode.c token.c error.c
OBJS = $(SRCS:.c=.o)

TARGET = parser

all: $(TARGET)

# Link chương trình
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

# Quy tắc build từng .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dọn dẹp
clean:
	rm -f *.o $(TARGET)

.PHONY: all clean
