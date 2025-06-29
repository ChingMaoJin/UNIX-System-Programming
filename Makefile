CC= gcc
EXEC= prime
FileName= Prime.c
$(EXEC): $(FileName)
	$(CC) $(FileName) -o $(EXEC) -lm

clean: 
	rm -f $(EXEC)