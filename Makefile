minls: minls.c minls.h
	gcc minls.c -o minls

minget: minget.c minls.h
	gcc minget.c -o minget

clean:
	rm -f minls minget