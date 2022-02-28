
#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "shareddefs.h"
#define MAX_LINE_LENGTH 300
int main(int argc, char *argv[])
{
	//MESSAGE QUEUE BETWEEN SERVER AND CLIENT 
	//variables
	mqd_t mq, mq2, mq3;
    struct mq_attr mq_attr,mq_attr2,mq_attr3;
	struct message *messageptr;
	struct message *messageptr2;
    int n,i,forkedid;
    char *bufptr;
	char *bufptr2;
	char *bufptr3;
	char *bufptr4;
    int buflen, buflen2,buflen4,buflen3;
	struct histdata *histdataptr;
	int intervalcount, intervalwidth, intervalstart;
	////recieve message from the client
	 mq = mq_open(CTOSMQ, O_RDWR | O_CREAT, 0666, NULL);
	

        if (mq == -1) {
                perror("can not create client to server msg queue\n");
                exit(1);
        }
        printf("client to server mq created, mq id = %d\n", (int) mq);
        mq_getattr(mq, &mq_attr);
        

        /* allocate large enough space for the buffer to store 
        an incoming message */
	
	 buflen = mq_attr.mq_msgsize;
	 bufptr = (char *) malloc(buflen);
	 n = mq_receive(mq, (char *) bufptr, buflen, NULL);
         if (n == -1) {
                perror("client to server mq_receive failed\n");
                exit(1);
         }

         printf("client to server mq_receive success, message size = %d\n", n);
	
        //free(bufptr);
      
	//CLIENT TO SERVER MESSAGE QUEUE ENDED
	messageptr = (struct message *) bufptr;
	free(bufptr);
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
	// find iteration time for both parents and children
	int iterationtime = ((int) (intervalcount / 501)) +1;
	//CHILDREN TO PAREN MESSAGE QUEUE OPEN
   	mq2 = mq_open(CTOPMQ, O_RDWR | O_CREAT, 0666, NULL);
    if (mq2 == -1) {
        perror("can not create children to parent msg queue\n");
        exit(1);
    }
    printf("in parent, children to parent mq created, mq2 id = %d\n", (int) mq2);
	
    for(i=1; i < argc; i++){
		forkedid =fork();
		if(forkedid == 0){
			// IT IS CHILD PROCESS
			//open the ith file from argv
			char* filename = argv[i];
			FILE* fp = fopen(filename,"r");
			if(fp == NULL){
				printf("Error: could not open file %s", filename);
        			return 1;
			}

   		 	char buffer[MAX_LINE_LENGTH];
		 	while (fgets(buffer, MAX_LINE_LENGTH, fp)){
				int num = atoi(buffer);
				//trace histogram
				int s;
				for(s = 0 ; s <intervalcount; s++){
					if(histogram[s].start_val<= num && histogram[s].end_val > num){
						histogram[s].count = histogram[s].count + 1;
					}
				}
			}
			// close the file
   		 	fclose(fp);
			//send by using message queue 
			int j; 
			for(j = 0 ; j < iterationtime; j++){
				//DO HERE!!!!!!!!!!!
				printf("iteration time for child id:%d is %d\n",getpid(),(iterationtime-j));
				//create histdata to send to parent (copy histogram)
				struct histdata histdata;
				histdata.pid = getpid();
				
				if(j < iterationtime -1 ){
					//replace full to the array
					int k;
					for(k = 0 ; k < 501; k++){
						histdata.histogram[k]= histogram[501*j+k].count;
					}
					histdata.start_val = histogram[501*j].start_val;
				}else if(j == iterationtime -1){
					//find modular of 501
					int mod = intervalcount % 501;
					int k;
					if(mod == 0){
						histdata.start_val = -1;
					}
					else{
						histdata.start_val = histogram[501*j].start_val;
					}
					for(k = 0 ; k < mod; k++){
						histdata.histogram[k] = histogram[501*j+k].count;
					}
					histdata.histogram[mod] = -1;
					for(k = mod +1 ; k < 1; k++){
						histdata.histogram[mod] = -2;
					}
				}
		
				//print histdata
				/*
				if(j < iterationtime -1){
					int m;
					for(m = 0 ; m < 501; m++){
						printf("in child with pid: %d, histogram::%d\n",getpid(),histdata.histogram[m]);
					}

				}else if(j == iterationtime -1){
					int mod = intervalcount % 501;
					int m;
					for(m = 0 ; m < mod; m++){
						printf("in child with pid: %d, histogram: :%d\n",getpid(),histdata.histogram[m]);
					}
					printf("last value of iteration count is:%d\n",histdata.histogram[mod]);
				}
				*/
				//look at the message count in the queue
				mq_getattr(mq2, &mq_attr2);
        		//printf("in child with pid:%d, current message size before send is %d\n", getpid(),(int) mq_attr.mq_curmsgs);
				//send histdata to the parent
				n = mq_send(mq2, (char*) &histdata,sizeof(struct histdata),0);
				if (n == -1) {
					perror("children to parent mq_send failed\n");
					exit(1);
				}
				printf("in children with id:%d, iteration:%d, chil to parent mq_send success, item size = %d\n",getpid(),(iterationtime-j),(int) sizeof(struct histdata));
				sleep(1);
			}
			exit(1);
		}
	}
		//in parent get the histdata from the child
		mq_getattr(mq2, &mq_attr2);
		int curmsg  = (int)mq_attr2.mq_curmsgs;
		int quit = 1;
		while( quit ||curmsg || (wait(NULL)>0)){
			if(curmsg != 0){
				
					quit = 0;
				
				buflen2 = mq_attr2.mq_msgsize;
				bufptr2 = (char *) malloc(buflen2);
				n = mq_receive(mq2, (char *) bufptr2, buflen2, NULL);

				if (n == -1) {
					perror("in parent, child to parent mq_receive failed\n");
					exit(1);
				}
				printf("in parent, child to parent mq_receive success, message size = %d\n", n);
				histdataptr = (struct histdata*) bufptr2;
				//printf("in parent, received histadat->pid = %d\n", histdataptr->pid);
				int p;
				int iteration_index_histogram = -1;
				//what is the start value of the iteration
				int start_val_iter = histdataptr->start_val;
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
					for(p = 0; p < 501; p++){
						if(histdataptr->histogram[p] == -1){
							break;
						}
						//sum the same iteration histadata arrays
						histogram[iteration_index_histogram + p].count = histogram[iteration_index_histogram + p].count + histdataptr->histogram[p];
						//printf("in parent, RECEIVED histogram: %d\n",histdataptr->histogram[p]);
					}
				}
				printf("\n");
				free(bufptr2);
			}
			mq_getattr(mq2, &mq_attr2);
			curmsg  = (int)mq_attr2.mq_curmsgs;
		
			printf("updated\n");
		}
		//finally print the histogram that is summed up
		printf("Final histogram counts are:\n");
		int fin;
		for(fin = 0; fin < intervalcount; fin++){
			printf("final parent histogram %d-%d: %d\n",histogram[fin].start_val,histogram[fin].end_val,histogram[fin].count);
		}	
		
		printf("All children terminated.\n");
		sleep(4);
		//MESSAGE QUEUE 3
		//send histogram data to the client
//------------------------------------------------------
		//first open message queueu3	
		mq3 = mq_open(STOCMQ, O_RDWR| O_CREAT, 0666, NULL);
		if (mq3 == -1) {
			perror("can not open server to client msg queue\n");
			exit(1);
		}
		printf("server to client mq opened, mq id = %d\n", (int) mq);
		
		int l; 
		for(l = 0 ; l < iterationtime; l++){
				//create histdata to send to client (copy histogram)
				struct histdata histdata2;
				histdata2.pid = getpid();
				
				if(l < iterationtime -1 ){
					//replace full to the array
					int t;
					for(t = 0 ; t < 501; t++){
						histdata2.histogram[t]= histogram[501*l+t].count;
					}
					histdata2.start_val = histogram[501*l].start_val;
				}else if(l == iterationtime -1){
					//find modular of 501
					int mod = intervalcount % 501;
					int t;
					if(mod == 0){
						histdata2.start_val = -1;
					}
					else{
						histdata2.start_val = histogram[501*l].start_val;
					}
					for(t = 0 ; t < mod; t++){
						histdata2.histogram[t] = histogram[501*l+t].count;
					}
					histdata2.histogram[mod] = -1;
					for(t = mod +1 ; t < 1; t++){
						histdata2.histogram[mod] = -2;
					}
				}
		
				//print histdata
				/*
				printf("start value of iteration: %d is %d\n",j,histdata2.start_val);
				if(l < iterationtime -1){
					int r;
					for(r = 0 ; r < 501; r++){
						printf("before send to client (%d-%d]: %d\n",histogram[l*501+r].start_val,histogram[l*501 +r].end_val,histdata2.histogram[r]);
					}

				}else if(l == iterationtime -1){
					int mod = intervalcount % 501;
					int r;
					for(r = 0 ; r < mod; r++){
						printf("in parent final histogram before send to client histogram last iteration: :%d\n",histdata2.histogram[r]);
					}
					printf("last value of iteration count is:%d\n",histdata2.histogram[mod]);
				}
				
				printf("size of histdata before send to parent is: %d\n",(int)sizeof(struct histdata));
				*/
				//look at the message count in the queue
				mq_getattr(mq3, &mq_attr3);
        		printf("in parent , current message size before send is %d\n",(int) mq_attr3.mq_curmsgs);
				//send histdata to the client
				n = mq_send(mq3, (char*) &histdata2,sizeof(struct histdata),0);
				if (n == -1) {
					perror("parent to client mq_send failed\n");
					exit(1);
				}
				printf("in parent iteration:%d, parent to client mq_send success, item size = %d\n",(iterationtime-l),(int) sizeof(struct histdata));
				sleep(1);
			}
			//take the message whether its done or not
			mq_getattr(mq, &mq_attr);
			buflen4 = mq_attr.mq_msgsize;
	 		bufptr4 = (char *) malloc(buflen4);
			n = mq_receive(mq, (char *) bufptr4, buflen4, NULL);
         	if (n == -1) {
                perror("done mq_receive failed\n");
                exit(1);
        	 }

         	printf("done mq_receive success, message size = %d\n", n);
			messageptr2 = (struct message *) bufptr4;
			char* msg2 = messageptr2->text;
			if(strcmp(msg2,"done") == 0){
				//clear all the messagequeues
				//printf end
				// clear mq1
				wait(NULL);
				int curmesg;
				mq_getattr(mq, &mq_attr);
				curmesg = mq_attr.mq_curmsgs;
				if(curmesg > 0){
					//clear mq
					while(curmesg > 0){
						buflen = mq_attr.mq_msgsize;
	 					bufptr = (char *) malloc(buflen);
	 					n = mq_receive(mq, (char *) bufptr, buflen, NULL);
						 printf("mq1 is cleaning...\n");
					}
					free(bufptr);
				}
				//clear mq2
				mq_getattr(mq2, &mq_attr2);
	    		curmesg  = (int)mq_attr2.mq_curmsgs;
				if(curmesg > 0){
					while(curmesg> 0){
						buflen2 = mq_attr2.mq_msgsize;
						bufptr2 = (char *) malloc(buflen2);
						n = mq_receive(mq2, (char *) bufptr2, buflen2, NULL);
						printf("mq2 is cleaning...\n");
					}
					free(bufptr2);
				}
				//clear mq3
				mq_getattr(mq3, &mq_attr3);
	    		curmesg  = (int)mq_attr3.mq_curmsgs;
				if(curmesg > 0){
					while(curmesg> 0){
						buflen3 = mq_attr3.mq_msgsize;
						bufptr3 = (char *) malloc(buflen3);
						n = mq_receive(mq3, (char *) bufptr3, buflen3, NULL);
						printf("mq3 is cleaning...\n");
					}
					free(bufptr3);
				}
				free(bufptr4);
				mq_close(mq);
				mq_close(mq2);
				mq_close(mq3);
				printf("server securely closed\n");
				return 1;
			}
			printf("server cannot securely closed");
			return -1;
        	
	
}
