CC=cc
CFLAGS= -g -Wall -Werror -pthread
TARGETS=data_to_kernel producer_consumer

all : $(TARGETS)

$(TARGETS): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	@rm -f $(TARGETS)
