.PHONY : all clean

all : h264-decoder.js

clean :
	rm -f h264-decoder.js
	cd openh264 && emmake make clean OS=linux ARCH=asmjs

h264-decoder.js : h264-decoder.cpp openh264/libopenh264.so
	emcc \
	  -O2 \
	  --bind \
	  --memory-init-file 0 \
	  -s VERBOSE=1 \
	  -s NO_FILESYSTEM=1 \
	  -s NO_BROWSER=1 \
	  -Iopenh264/codec/api/svc \
	  -o h264-decoder.js \
	  h264-decoder.cpp \
	  openh264/libopenh264.so

openh264/libopenh264.so :
	cd openh264 && emmake make OS=linux ARCH=asmjs
