CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -pedantic -g
TARGET  = hotel
SRCDIR  = src
OBJDIR  = obj

SRCS    = $(wildcard $(SRCDIR)/*.c)
OBJS    = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

.PHONY: all clean run dirs

all: dirs $(TARGET)

dirs:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	@if not exist data mkdir data
	@if not exist reports mkdir reports

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	@if exist $(OBJDIR) rmdir /s /q $(OBJDIR)
	@if exist $(TARGET).exe del $(TARGET).exe
	@if exist $(TARGET) del $(TARGET)
