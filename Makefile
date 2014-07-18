CFLAGS = -O0 -g -Wall -W -std=gnu99 `pkg-config --cflags libusb-1.0`
LDFLAGS = -g -Wall -std=gnu99 `pkg-config --libs libusb-1.0`
ZEROMINUS_OBJS = analyzer.o gl.o vcd.o zerominus.o
ZEROPROG_OBJS = gl.o zeroprog.o

all: zerominus zeroprog

clean:
	-rm -f zerominus zeroprog $(ZEROMINUS_OBJS) $(ZEROPROG_OBJS)

zerominus: $(ZEROMINUS_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

zeroprog: $(ZEROPROG_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
