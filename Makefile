.PHONY: all clean

all:
	ninja -C build install -v

clean:
	rm -rf build
