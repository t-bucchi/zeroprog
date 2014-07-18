CFLAGS = -O2 -g -Wall -W -std=gnu99 `pkg-config --cflags libusb-1.0`
LDFLAGS = -g -Wall -std=gnu99 `pkg-config --libs libusb-1.0`
ZEROPROG_OBJS = gl.o zeroprog.o

all: zeroprog

clean:
	-rm -f zerominus zeroprog $(ZEROMINUS_OBJS) $(ZEROPROG_OBJS)

zeroprog: $(ZEROPROG_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
