OBJS=fdm_v.o fdm_a.o fdm_h.o

CFLAGS=-Wall -g
LIB=fdm.a

$(LIB): $(OBJS)
	ar rv $(LIB) $(OBJS)

%.o: %.c
	$(CC) $(DEFS) $(CFLAGS) -c $<

# dependencies
-include makefile.depend

dep:
	gcc $(DEFS) -MM *.c > makefile.depend

clean:
	rm -f *.o *.a tags
