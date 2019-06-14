#arm-elf-gcc -DMEMWATCH -DMW_STDIO
export CC := arm-elf-gcc
export CFLAGS := -Wall -O3 -pipe


TARGET := mcpu


LOCAL_OBJS := $(patsubst %.c, %.o, $(wildcard *.c))


.PHONY : all
all : $(TARGET)

$(TARGET) : $(LOCAL_OBJS)
	$(CC) $(CFLAGS) -o $@ $+ -elf2flt -lpthread


%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@


.PHONY : clean
clean :
	rm -f $(TARGET) *.o *.gch *.gdb *.elf *.elf2flt *.bak
