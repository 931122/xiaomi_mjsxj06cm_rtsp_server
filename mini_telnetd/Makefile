CC = /opt/toolchain/arm-sigmastar-linux-uclibcgnueabihf-9.1.0/bin/arm-sigmastar-linux-uclibcgnueabihf-9.1.0-gcc

TARGET = telnetd

all: $(TARGET)

CFLAGS = -O2 -Wall
# Static compile
#CFLAGS = -O2 -Wall -static -static-libgcc
# Use for older systems 
#CFLAGS = -O2 -Wall -DOLD_GETPTY=true

LDFLAGS= -s
OBJS = mini_telnetd.o

$(TARGET): $(OBJS)
		$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
		rm $(OBJS) $(TARGET)

