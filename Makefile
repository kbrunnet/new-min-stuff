all: minls minget

minls: minls.c minls.h minCommon.h
	gcc minls.c -o minls

minget: minget.c minget.h minCommon.h
	gcc minget.c -o minget

clean:
	rm -f minls minget
