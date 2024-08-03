

OBJS = mi_rtsp_server.o rtsp_demo.o rtsp_msg.o rtp_enc.o stream_queue.o utils.o
TAR = mi_rtsp_server

CC = /opt/toolchain/arm-sigmastar-linux-uclibcgnueabihf-9.1.0/bin/arm-sigmastar-linux-uclibcgnueabihf-9.1.0-gcc
CFLAGS += -g -Wall -D__LINUX__ -s

all: $(TAR)

$(TAR) : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lrt

%.o : %.c
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f $(OBJS) $(TAR)
