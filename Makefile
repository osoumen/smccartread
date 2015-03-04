# Command-line client
CMDLINE = smcartread

# By default, build the firmware and command-line client
all: $(CMDLINE)

# One-liner to compile the command-line client
$(CMDLINE): smcartread.c rs232.c
	gcc -O -Wall smcartread.c rs232.c -o smcartread

# Housekeeping if you want it
clean:
	$(RM) *.exe
