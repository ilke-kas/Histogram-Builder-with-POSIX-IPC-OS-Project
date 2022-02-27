#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "shareddefs.h"

int main(int argc, char *argv[]){	
	// take the command line arguments
	mqd_t mq,mq3;
	printf("You take %d arguments and these are:\n",argc);
	printf("arg[0]:%s,arg[1]:%s,arg[2]:%s,arg[3]:%s\n",argv[0],argv[1],argv[2],argv[3]);
        int intervalcount = atoi(argv[1]);
        int intervalwidth = atoi(argv[2]);
        int intervalstart = atoi(argv[3]);
        printf("interval count is :%d\n", intervalcount);
        //create histogram
        int j;
        struct histinterval histogram[intervalcount];
        for(j= 0; j < intervalcount; j++){
                histogram[j].count = 0;
                histogram[j].width =intervalwidth;
                if(j == 0){
                        histogram[j].start_val = intervalstart;
                }
                else{
                        histogram[j].start_val = histogram[j-1].end_val;
                }
                histogram[j].end_val = histogram[j].start_val + histogram[j].width;
        }
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
	mq = mq_open(CTOSMQ, O_RDWR| O_CREAT, 0666, NULL);
    if (mq == -1) {
        perror("can not open msg queue 1\n");
        exit(1);
    }
    printf("mq opened, mq id = %d\n", (int) mq);
	 mq3 = mq_open(STOCMQ, O_RDWR | O_CREAT, 0666, NULL);
    if (mq3 == -1) {
        perror("can not create server to client  msg queue\n");
        exit(1);
	}
    printf("server to client mq created, mq3 id = %d\n", (int) mq3);
    struct message message;
    int n;
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
	
	sleep(15);
    //open message queue 3
    struct mq_attr mq_attr3;
    struct histdata *histdataptr;
    int n3;
    char *bufptr3;
    int buflen3;
    mq_getattr(mq3, &mq_attr3);
	printf("mq maximum msgsize = %d\n", (int) mq_attr3.mq_msgsize);
    int curmsg = (int)mq_attr3.mq_curmsgs;
    printf("client curmsg is:%d\n",curmsg);
    //while loop

    while( curmsg > 0){
		printf("inside current messages are:%d\n",curmsg);
		if(curmsg != 0){
			buflen3 = mq_attr3.mq_msgsize;
			bufptr3 = (char *) malloc(buflen3);
			n3 = mq_receive(mq3, (char *) bufptr3, buflen3, NULL);

			if (n3 == -1) {
				perror("in client, server to client mq_receive failed\n");
				exit(1);
			}
			printf("in client, server to client mq_receive success, message size = %d\n", n3);
			histdataptr = (struct histdata*) bufptr3;
			printf("in client, received histadata->pid = %d\n", histdataptr->pid);
			
			int iteration_index_histogram = -1;
			//what is the start value of the iteration
			int start_val_iter = histdataptr->start_val;
			printf("HERE IS THE START VALUE OF THE ITERATION : %d\n",start_val_iter);
			//find the appropiate start according to iteration
			//trace histogram and find index of value of the start value which is same with the iteration and the count
			int q;
			for(q = 0 ; q < intervalcount; q++){
				if(histogram[q].start_val == start_val_iter){
					iteration_index_histogram = q;
					break;
				}
			}
						
			//index of this start val in histogram
			if(start_val_iter != -1){
				int p;
				for(p = 0; p < 301; p++){
					if(histdataptr->histogram[p] == -1){
						break;
					}
				//sum the same iteration histadata arrays
				histogram[iteration_index_histogram + p].count =  histdataptr->histogram[p];
				printf("in parent, received histogram: %d\n",histdataptr->histogram[p]);
				}
			}
			printf("\n");
			free(bufptr3);
		}
		mq_getattr(mq3, &mq_attr3);
		curmsg  = (int)mq_attr3.mq_curmsgs;
		printf("updated\n");
    }
    //finally print the histogram that is summed up
	printf("Final histogram counts are:\n");
	int fin;
	for(fin = 0; fin < intervalcount; fin++){
		printf("final parent histogram (%d,%d]: %d\n",histogram[fin].start_val,histogram[fin].end_val,histogram[fin].count);
	}	
	mq_close(mq3);
	//send message that client is done
	struct message message2;
    message2.id = 1;
    strcpy(message2.text, "done");

    n = mq_send(mq, (char *) &message2, sizeof(struct message), 0);

    if (n == -1) {
        perror("done mq_send failed\n");
        exit(1);
    }
	printf("done mq_send success, message size = %d\n",
                       (int) sizeof(struct message));
    printf("done item->id   = %d\n", message2.id);
    printf("doneitem->astr = %s\n", message2.text);
    printf("\n");

	mq_close(mq);
        
    return 1;
}
