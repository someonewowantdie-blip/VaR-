CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -pedantic
LDFLAGS ?= -lm

SRCS = main.c stats.c var_engine.c backtest.c csv_loader.c
OBJS = $(SRCS:.c=.o)
TARGET = var_engine

.PHONY: all clean gen-sample test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

gen-sample: gen_sample_data.c
	$(CC) $(CFLAGS) -o gen_sample_data gen_sample_data.c $(LDFLAGS)
	./gen_sample_data > sample_returns.csv

test: all gen-sample
	./$(TARGET) sample_returns.csv 0.99 1000000 250

clean:
	rm -f $(OBJS) $(TARGET) gen_sample_data sample_returns.csv