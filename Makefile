minls: minls.c super.h partition.h
	gcc minls.c -o minls

minget: minget.c super.h partition.h
	gcc minget.c -o minget

clean:
	rm -f minls minget