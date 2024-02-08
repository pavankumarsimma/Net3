POP=3000
SMTP=3400
all: pop3_server 
	
pop3_server:
	gcc pop.c -o pop
	./pop ${POP}
client:
	cd ../client/; gcc c.c -o c
clean: 
	rm pop s rough
