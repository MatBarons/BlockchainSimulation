CC = gcc
CFLAGS = -std=c89 -pedantic -g -Wall 
COMMON_DEPS = ../Headers/*.h

all: master node user

master: master.c $(COMMON_DEPS)
	$(CC) $(CFLAGS) common_ipcs.c $< -o master

node: node.c $(COMMON_DEPS)
	$(CC) $(CFLAGS) common_ipcs.c $< -o node

user: user.c $(COMMON_DEPS)
	$(CC) $(CFLAGS) common_ipcs.c $< -o user

run:
	make all
	./master  ../files_needed/Configs.txt ../files_needed/keys

clean:
	$(RM) user
	$(RM) node
	$(RM) master




	

