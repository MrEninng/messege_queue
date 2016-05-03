#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define BUFF_SIZE 512
#define CHAR_BUFF_SIZE 26

struct message {
	long mtype;
	char mtext[BUFF_SIZE];
};

int parse(const char *buff);

void daughter1(int msqid, char* buff, int it);
void daughter2(int msqid, char* buff, int it);

int main(int argc, char **argv)
{
	int msqid = msgget(IPC_PRIVATE, 0666); // Как ключ передается флаг IPC_PRIVATE, т.к. в данной задаче используются родственные процессы
	//поэтому не нужно создавать какой-то файл на диске и использовать функцию ftok. Cоответсвенно вторым параметром передаются только 
	// права доступа, т.к. не нужно в этом случаее что либо создавать.
	if (msqid < 0) {
		perror("msgget");
		exit(errno);
	}
	char *buff = malloc(sizeof(char) * CHAR_BUFF_SIZE);
	for (int i = 0; i < CHAR_BUFF_SIZE; ++i) {
		buff[i] = 97 + i;
	}
	int it, it2;
	printf("Enter iter amount for daughter1: ");
	scanf("%d",&it); 
	printf("\nEnter iter amount for daughter2: ");
	scanf("%d",&it2); 
	puts("");
	
	daughter1(msqid, buff, it);
	daughter2(msqid, buff, it2);
	
	int res;
	int flag = 2;
	struct message msg;
	while(flag) {
		res = msgrcv(msqid, &msg, sizeof(char) * BUFF_SIZE, 1, 0);
		if (res < 0) {
			perror("msgrcv");
			exit(errno);
		}
		res =  strcmp(msg.mtext, "D1 - die");
		if (res == 0) {
			puts(msg.mtext);
			--flag;
			sleep(1);
			continue;
		}
		res =  strcmp(msg.mtext, "D2 - die");
		if (res == 0) {
			puts(msg.mtext);
			sleep(1);
			--flag;
			continue;
		}
		
		printf("Parent: got from %s\n", msg.mtext);
		
		int val = parse(msg.mtext);
		buff[val] = '#';
		if (msg.mtext[1] == '1') {
			msg.mtype = 2;
		} else if (msg.mtext[1] == '2') {
			msg.mtype = 3;
		}
		sprintf(msg.mtext, "#");	
		res = msgsnd(msqid, &msg, sizeof(msg.mtext),0);
		if (res < 0) {
			perror("msgsnd");
			exit(errno);
		}
	}
	
	res = msgctl(msqid, IPC_RMID, NULL);
	if (res < 0) {
		perror("msgctl:RMID");
		exit(errno);
	}
	
	puts("***\nChanged mass\n***");
	for (int i = 0; i < CHAR_BUFF_SIZE; ++i) {
		printf("i = %d: %c\n", i, buff[i]);
	}	
	return 0;
}

void daughter1(int msqid, char *buff, int it)
{
	pid_t pid;
	switch (pid = fork()) {
		case -1 : {
			perror("D1 fork");
			exit(errno);
		}
		case 0 : {
			srand(100);
			struct message msg;
			
			for (int i = 0; i < it; ++i) {
				int r = rand() % 6;
				sleep(r);
				r = rand() % 26;
				sprintf(msg.mtext, "D1: %d", r);
				msg.mtype = 1;
				int res = msgsnd(msqid, &msg, sizeof(msg.mtext), 0);
				if (res < 0) {
					perror("D1 msgsnd");
					exit(errno);
				}
				res = msgrcv(msqid, &msg, sizeof(char) * BUFF_SIZE, 2, 0);
				if (res < 0) {
					perror("msgrcv");
					exit(errno);
				}
				buff[r] = msg.mtext[0];
				printf("D1: %c\n", buff[r]);
			}
			sprintf(msg.mtext, "D1 - die");
			msg.mtype = 1;
			int res = msgsnd(msqid, &msg, sizeof(msg.mtext), 0);
			
			if (res < 0) {
				perror("D1 msgsnd");
				exit(errno);
			}
			exit(EXIT_SUCCESS);
		}
		
		default : 
			break;
	}
}

void daughter2(int msqid, char *buff, int it)
{
	pid_t pid;
	switch (pid = fork()) {
		case -1 : {
			perror("D2 fork");
			exit(errno);
		}
		case 0 : {
			srand(100);
			struct message msg;
			srand(time(NULL));
			
			for (int i = 0; i < it; ++i) {
				int r = rand() % 3;
				sleep(r);
				r = rand() % 26;
				sprintf(msg.mtext, "D2: %d", r);
				msg.mtype = 1;
				int res = msgsnd(msqid, &msg, sizeof(msg.mtext), 0);
				if (res < 0) {
					perror("D2 msgsnd");
					exit(errno);
				}
				res = msgrcv(msqid, &msg, sizeof(char) * BUFF_SIZE, 3, 0);
				if (res < 0) {
					perror("msgrcv");
					exit(errno);
				}
				buff[r] = msg.mtext[0];
				printf("D2: %c\n", buff[r]);
			}
			sprintf(msg.mtext, "D2 - die");
			msg.mtype = 1;
			int res = msgsnd(msqid, &msg, sizeof(msg.mtext), 0);
			
			if (res < 0) {
				perror("D2 msgsnd");
				exit(errno);
			}
			exit(EXIT_SUCCESS);
		}
		
		default : 
			break;
	}
}

int parse(const char *buff)
{
	int value = -1;
	char c;
	sscanf(buff, "D%c: %d", &c, &value);
	//fflush(stdout);
	return value;
}
