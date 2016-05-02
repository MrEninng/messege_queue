#include <stdio.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define QUEUE_NAME "/myQueue.mq"
#define D1_QUEUE_NAME "/d1Queue.mq"
#define D2_QUEUE_NAME "/d2Queue.mq"

#define DT1_BUFF_SIZE 26
#define BUFF_SIZE = 1024;

void daughter1(char *buff, long size, int it);
void daughter2(char *buff, long size, int it);

int parse(const char *buff);

int main(int argc, char **argv)
{
	mqd_t mqd = mq_open(QUEUE_NAME, O_RDONLY | O_CREAT, 0666, NULL);
	if (mqd < 0) {
		perror("mq_open");
		exit(errno);
	}
	mqd_t mqd_d1 = mq_open(D1_QUEUE_NAME, O_WRONLY | O_CREAT, 0666, NULL);
	if (mqd_d1 < 0) {
		perror("mq_open d1");
		exit(errno);
	}
	mqd_t mqd_d2 = mq_open(D2_QUEUE_NAME, O_WRONLY | O_CREAT, 0666, NULL);
	if (mqd_d2 < 0) {
		perror("mq_open d2");
		exit(errno);
	} 
	
	struct mq_attr obuf;
	
	int ret = mq_getattr(mqd, &obuf);
	if (ret < 0) {
		perror("mq_getattr");
		exit(errno);
	}
	
	char *buff = malloc(sizeof(char) * DT1_BUFF_SIZE);
	for (int i = 0; i < DT1_BUFF_SIZE; ++i) {
		buff[i] = 97 + i;
	}
	/*for (int i = 0; i < DT1_BUFF_SIZE; ++i) {
		printf("dt1 i = %d: %c\n", i, buff[i]);
	} */
	int it, it2;
	printf("Enter iter amount for daughter1: ");
	scanf("%d",&it); 
	printf("\nEnter iter amount for daughter2: ");
	scanf("%d",&it2); 
	puts("");
	
	daughter1(buff, obuf.mq_msgsize, it);
	daughter2(buff, obuf.mq_msgsize, it2);
	

	char *msg = malloc( sizeof(char) * obuf.mq_msgsize);
	memset(msg, '\0', obuf.mq_msgsize);
	int flag = 2; // С каждой смерти дочернего процесса будет вычитаться 1
	while (flag) {
		ret = mq_receive(mqd, msg, obuf.mq_msgsize, NULL); 
		if (ret < 0) {
			perror("mq_receive");
			exit(errno);
		}
		int res;
		//Проверка что дочери не погибли
		if ((res = strcmp(msg, "d1 - die")) == 0) {
			--flag;
			puts(msg);
			continue;
		}
		if ((res = strcmp(msg, "d2 - die")) == 0) {
			--flag;
			puts(msg);
			continue;
		}
		printf("Parent: Got from %s\n", msg);	
		int val = parse(msg);
		//printf("Parant: VAL = %d\n", val);
		buff[val] = '#';
		fflush(stdout);
		// Отправка сообщений обратно дочерям
		if (msg[1] == '1') {
			ret = mq_send(mqd_d1, "#", sizeof("#"), 0);
			if (ret < 0) {
				perror("mq_send");
				exit(errno);
			}
		} else if (msg[1] == '2') {
			ret = mq_send(mqd_d2, "#", sizeof("#"), 0);
			if (ret < 0) {
				perror("mq_send");
				exit(errno);
			}
		}
	}
	
	int status;
	pid_t w;
	while ((w = wait(&status)) != -1) {} 
	mq_close(mqd);
	mq_unlink(QUEUE_NAME);
	mq_close(mqd_d1);
	mq_unlink(D1_QUEUE_NAME);
	mq_close(mqd_d2);
	mq_unlink(D2_QUEUE_NAME);
	
	puts("***\nChanged mass\n***");
	for (int i = 0; i < DT1_BUFF_SIZE; ++i) {
		printf("i = %d: %c\n", i, buff[i]);
	}
	
	free(buff);
	free(msg);
	return 0;
}

void daughter1(char *buff, long msgsize, int it) 
{
	pid_t pid;
	switch(pid = fork()) {
		case -1 : {
			perror("fork doughter1");
			exit(errno);
		}
		case 0 : {
			mqd_t mqd = mq_open(QUEUE_NAME, O_WRONLY);
			if (mqd < 0) {
				perror("D1: mq_open");
				exit(errno);
			}
			mqd_t mqd_rd = mq_open(D1_QUEUE_NAME, O_RDONLY);
			if (mqd_rd < 0) {
				perror("D1: mq_open_rd");
				exit(errno);
			}
			printf("Daughter1: %d\n", getpid());
		
			srand(time(NULL));
			for (int i = 0; i < it; ++i) {
				int r = rand() % 2;
				sleep(r);
								
				r = rand() % 26;
				char msg[msgsize];
				sprintf(msg, "d1: %d", r);
				int ret = mq_send(mqd, msg, sizeof(msg), 0);
				if (ret < 0) {
					perror("mq_send_d1");
					exit(errno);
				}
				ret = mq_receive(mqd_rd, msg, msgsize, NULL);
				if (ret < 0) {
					perror("mq_receive_d1");
					exit(errno);
				}
				printf("D1: %s\n", msg);
				buff[r] = msg[0];
			}
			int ret = mq_send(mqd, "d1 - die", sizeof("d1 - die"), 0);
			if (ret < 0) {
				perror("mq_send_die_d1");
				exit(errno);
			}
			sleep(1);
			mq_close(mqd);
			mq_close(mqd_rd);
			fflush(stdout); // Флашим стдоаут т.к. иначе выхлоп 
							//будет только после смерти дочернего процесса
			exit(EXIT_SUCCESS);
		}
		default : {
			break;
		}
	}
}

void daughter2(char *buff, long msgsize, int it) 
{
	pid_t pid;
	switch(pid = fork()) {
		case -1 : {
			perror("fork doughter2");
			exit(errno);
		}
		case 0 : {
			mqd_t mqd = mq_open(QUEUE_NAME, O_WRONLY);
			if (mqd < 0) {
				perror("D2: mq_open");
				exit(errno);
			}
			mqd_t mqd_rd = mq_open(D2_QUEUE_NAME, O_RDONLY);
			if (mqd_rd < 0) {
				perror("D2: mq_open_rd");
				exit(errno);
			}
			printf("Daughter2: %d\n", getpid());
		
			srand(26);
			for (int i = 0; i < it; ++i) {
				int r = rand() % 3;
				sleep(r);
								
				r = rand() % 26;
				char msg[msgsize];
				sprintf(msg, "d2: %d", r);
				int ret = mq_send(mqd, msg, sizeof(msg), 0);
				if (ret < 0) {
					perror("mq_send_d2");
					exit(errno);
				}
				ret = mq_receive(mqd_rd, msg, msgsize, NULL);
				if (ret < 0) {
					perror("mq_receive_d2");
					exit(errno);
				}
				printf("d2: %s\n", msg);
				buff[r] = msg[0];
			}
			int ret = mq_send(mqd, "d2 - die", sizeof("d2 - die"), 0);
			if (ret < 0) {
				perror("mq_send_die_d2");
				exit(errno);
			}
			sleep(1);
			mq_close(mqd);
			mq_close(mqd_rd);
			fflush(stdout); // Флашим стдоаут т.к. иначе выхлоп 
							//будет только после смерти дочернего процесса
			exit(EXIT_SUCCESS);
		}
		default : {
			break;
		}
	}
}

int parse(const char *buff)
{
	int value = -1;
	char c;
	sscanf(buff, "d%c: %d", &c, &value);
	fflush(stdout);
	return value;
}
