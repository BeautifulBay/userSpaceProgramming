
all: framebuffer.o
	@echo "Done!"

framebuffer.o: framebuffer.c key_value_to_ascii2.h key_value_to_ascii.h font_8x16.h
	gcc $< -o $@ -lpthread

clean:
	rm -rf *.o
