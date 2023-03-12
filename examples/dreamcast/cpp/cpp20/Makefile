#
# C++ out-of-memory demonstration
# (c)2023 Falco Girgis
#   

TARGET = cpp20.elf
OBJS = module.o cpp20.o 
KOS_CPPFLAGS += -std=c++23 -fmodules-ts 

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS) 

rm-elf:
	-rm -f $(TARGET) 

$(TARGET): $(OBJS) 
	kos-c++ -o $(TARGET) $(OBJS) -lm

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

