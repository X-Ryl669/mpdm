OBJS=fdm_v.o fdm_a.o fdm_h.o fdm_d.o fdm_s.o

CFLAGS=-Wall -g
LIB=libfdm.a

$(LIB): $(OBJS)
	$(AR) rv $(LIB) $(OBJS)

%.o: %.c
	$(CC) $(DEFS) $(CFLAGS) -c $<

# dependencies
-include makefile.depend

dep:
	gcc $(DEFS) -MM *.c > makefile.depend

clean:
	rm -f *.o *.a tags stress

stress: stress.c $(LIB)
	$(CC) $(CFLAGS) $< -L. -lfdm -o $@
