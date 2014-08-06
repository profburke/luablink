


blink: luablink.c
	gcc -DUSE_HIDAPI -bundle -undefined dynamic_lookup -o luablink.so luablink.c -lBlink1


clean:
	rm -f *.o *.so *~

