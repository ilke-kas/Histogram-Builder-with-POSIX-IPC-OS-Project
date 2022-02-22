#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "shareddefs.h"

int main(int argc, char *argv[]){	
	// take the command line arguments
	printf("You take %d arguments and these are:\n",argc);
	printf("arg[0]:%s,arg[1]:%s,arg[2]:%s,arg[3]:%s\n",argv[0],argv[1],argv[2],argv[3]);
	//send message to the server
	char msg[strlen(argv[1])];
	strcpy(msg,argv[1]);
	strcat(msg, " ");
	strcat(msg, argv[2]);
	strcat(msg," ");
	strcat(msg,argv[3]);	
	
	//print the message
	printf("Message is %s\n",msg);
	//send to the server
	mqd_t mq;
        struct message message;
        int n;
        mq = mq_open(CTOSMQ, O_RDWR);
        if (mq == -1) {
                perror("can not open msg queue\n");
                exit(1);
        }
        printf("mq opened, mq id = %d\n", (int) mq);
        int i = 0;
                message.id = i;
                strcpy(message.text, msg);

                n = mq_send(mq, (char *) &message, sizeof(struct message), 0);

                if (n == -1) {
                        perror("mq_send failed\n");
                        exit(1);
                }

                printf("mq_send success, message size = %d\n",
                       (int) sizeof(struct message));
                printf("item->id   = %d\n", message.id);
                printf("item->astr = %s\n", message.text);
                printf("\n");
                i++;
                sleep(5);
		mq_close(mq);
        	return 1;

}
