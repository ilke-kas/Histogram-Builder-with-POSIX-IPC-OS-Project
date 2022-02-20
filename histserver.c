
#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "shareddefs.h"
#define MAX_LINE_LENGTH 300
int main(int argc, char *argv[])
{
	//variables
	mqd_t mq;
        struct mq_attr mq_attr;
	struct message *messageptr;
        int n,i;
        char *bufptr;
        int buflen;
	int intervalcount, intervalwidth, intervalstart;
	////recieve message from the client
	 mq = mq_open(CTOSMQ, O_RDWR | O_CREAT, 0666, NULL);
        if (mq == -1) {
                perror("can not create msg queue\n");
                exit(1);
        }
        printf("mq created, mq id = %d\n", (int) mq);

        mq_getattr(mq, &mq_attr);
        printf("mq maximum msgsize = %d\n", (int) mq_attr.mq_msgsize);

        /* allocate large enough space for the buffer to store 
        an incoming message */
	
	 buflen = mq_attr.mq_msgsize;
	 bufptr = (char *) malloc(buflen);
	 n = mq_receive(mq, (char *) bufptr, buflen, NULL);
         if (n == -1) {
                perror("mq_receive failed\n");
                exit(1);
         }

         printf("mq_receive success, message size = %d\n", n);
	
        free(bufptr);
        mq_close(mq);

	messageptr = (struct message *) bufptr;
	//get arguments from the recieved message text
	char* msg = messageptr->text;
	const char s[1] = " ";
	char *token;
	token = strtok(msg,s);
	intervalcount = atoi(token);
	token = strtok(NULL,s);
	intervalwidth = atoi(token);
	token = strtok(NULL,s);
	intervalstart = atoi(token);
	printf("count: %d, width:%d, start:%d", intervalcount,intervalwidth,intervalstart);
	// create histogram
	struct histinterval histogram[intervalcount];
	for(i= 0; i < intervalcount; i++){
		histogram[i].count = 0;
		histogram[i].width =intervalwidth;
		if(i == 0){
			histogram[i].start_val = intervalstart;
		}
		else{
			histogram[i].start_val = histogram[i-1].end_val;
		}
		histogram[i].end_val = histogram[i].start_val + histogram[i].width;
	}
	//print the histogr4am
	 for(i= 0; i <intervalcount; i++){ 
            printf("%d -%d: %d\n",histogram[i].start_val,histogram[i].end_val,histogram[i].count);
        }
	//now take the input file namesaw
	printf("There are %d file in histserver\n",argc);
	for(i=1; i < argc; i++){
		n =fork();
		if(n == 0){
		// IT IS CHILD PROCESS
		//open the ith file from argv
		
			char* filename = argv[i];
			printf("file name is %s i is %d n is %d\n",filename,i,n);
			FILE* fp = fopen(filename,"r");
			if(fp == NULL){
				printf("Error: could not open file %s", filename);
        			return 1;
			}

   		 	char buffer[MAX_LINE_LENGTH];
			printf("----------------------");
		 	while (fgets(buffer, MAX_LINE_LENGTH, fp)){
				int num = atoi(buffer);
				printf("this is the num: %d\n",num);
				//trace histogram
				for(i = 0 ; i <intervalcount; i++){
					if(histogram[i].start_val<= num && histogram[i].end_val > num){
						histogram[i].count = histogram[i].count + 1;
					}
				}

			}
			 //print the histogr4am
                         for(i= 0; i <intervalcount; i++){ 
                                 printf("%d -%d: %d\n",histogram[i].start_val, histogram[i].end_val, histogram[i].count);
                         }

   		 	// close the file
   		 	fclose(fp);
			//send values to parent by using message queue
			exit(1);
		}
	}
		for(i = 0 ; i < argc; i++)
			wait(NULL);
		printf("All children terminated.\n");
        	return 0;
	
}
