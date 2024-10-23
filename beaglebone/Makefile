backscatter: ble.o blebackscatter.o  pru.o
	gcc blebackscatter.o ble.o pru.o -o blebackscatter -lpthread -lprussdrv


pru: pru.c
	gcc pru.c -c pru.o

ble: ble.c
	gcc ble.c -c ble.o

blebackscatter: blebackscatter.c
	gcc blebackscatter -c blebackscatter.o

clean:
	rm -f backscatter *.o 
