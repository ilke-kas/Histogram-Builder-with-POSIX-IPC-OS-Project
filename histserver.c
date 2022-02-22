
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
	mqd_t mq, mq2;
    struct mq_attr mq_attr,mq_attr2;
	struct message *messageptr;
    int n,i,forkedid;
    char *bufptr;
	char *bufptr2;
    int buflen, buflen2;
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
        printf("client to server mq maximum msgsize = %d\n", (int) mq_attr.mq_msgsize);

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
	
        free(bufptr);
        mq_close(mq);
	//CLIENT TO SERVER MESSAGE QUEUE ENDED
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
	int iterationtime = ((int) (intervalcount / 201)) +1;
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
			printf("size of arr with 1000 int: %d\n", (int)sizeof(int[1000]));
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
				printf("iteration time for child id:%d is %d\n",getpid(),iterationtime-j);
				//create histdata to send to parent (copy histogram)
				struct histdata histdata;
				histdata.pid = getpid();
				if(j < iterationtime -1 ){
					//replace full to the array
					int k;
					for(k = 0 ; k < 201; k++){
						histdata.histogram[k]= histogram[201*j+k].count;
					}
				}else if(j == iterationtime -1){
					//find modular of 201
					int mod = intervalcount % 201;
					int k;
					for(k = 0 ; k < mod; k++){
						histdata.histogram[k] = histogram[201*j+k].count;
					}
					histdata.histogram[mod] = -1;
					for(k = mod +1 ; k < 1; k++){
						histdata.histogram[mod] = -2;
					}
				}
		
				//print histdata
				printf("in child with pid: %d, in parent is : %d value of histdata.pid should be same %d\n", getpid(),getppid(), histdata.pid);
				
				if(j < iterationtime -1){
					int m;
					for(m = 0 ; m < 201; m++){
						printf("in child with pid: %d, histogram::%d\n",getpid(),histdata.histogram[m]);
					}

				}else if(j == iterationtime -1){
					int mod = intervalcount % 201;
					int m;
					for(m = 0 ; m < mod; m++){
						printf("in child with pid: %d, histogram: :%d\n",getpid(),histdata.histogram[m]);
					}
					printf("last value of iteration count is:%d\n",histdata.histogram[mod]);
				}
				
				printf("size of histdata before send to parent is: %d\n",(int)sizeof(struct histdata));
				//look at the message count in the queue
				mq_getattr(mq, &mq_attr);
        		printf("in child with pid:%d, current message size before send is %d\n", getpid(),(int) mq_attr.mq_curmsgs);
			/*	if(mq_attr.mq_curmsgs >= 8){
					srand(time(NULL));   // Initialization, should only be called once.
					int r = rand() % 10; 
					sleep(r);
				}*/
				//send histdata to the parent
				n = mq_send(mq2, (char*) &histdata,sizeof(struct histdata),0);
				if (n == -1) {
					perror("children to parent mq_send failed\n");
					exit(1);
				}
				printf("in children with id:%d, iteration:%d, chil to parent mq_send success, item size = %d\n",getpid(),(iterationtime-j),(int) sizeof(struct histdata));

			}
			
			exit(1);
		}
	}
	/*	//clear message queue part
		mq_getattr(mq2, &mq_attr2);
		int curmsg  = (int)mq_attr2.mq_curmsgs;
		while(curmsg> 0){
			printf("curmsg is:%d\n",curmsg);
			buflen2 = mq_attr2.mq_msgsize;
			bufptr2 = (char *) malloc(buflen2);

			n = mq_receive(mq2, (char *) bufptr2, buflen2, NULL);

		}
		wait(NULL);*/
		//in parent get the histdata from the child
		mq_getattr(mq2, &mq_attr2);
		int curmsg  = (int)mq_attr2.mq_curmsgs;
		printf("outside curmsg is:%d\n",curmsg);
		
		while(curmsg || (wait(NULL) > 0)){
			
			printf("in parent, mq maximum msgsize = %d\n", (int) mq_attr2.mq_msgsize);
			printf("inside current messages are:%d\n",curmsg);
			if(curmsg != 0){
				buflen2 = mq_attr2.mq_msgsize;
				bufptr2 = (char *) malloc(buflen2);
				n = mq_receive(mq2, (char *) bufptr2, buflen2, NULL);

				if (n == -1) {
					perror("in parent, child to parent mq_receive failed\n");
					exit(1);
				}
				printf("in parent, child to parent mq_receive success, message size = %d\n", n);
				histdataptr = (struct histdata*) bufptr2;
				printf("in parent, received histadat->pid = %d\n", histdataptr->pid);
				int p;
				for(p = 0 ; p < 201; p++){
					if(histdataptr->histogram[p] == -1){
						break;
					}
					printf("in parent, received histogram: %d\n",histdataptr->histogram[p]);
				}
				printf("\n");
				free(bufptr2);
			}
			mq_getattr(mq2, &mq_attr2);
			curmsg  = (int)mq_attr2.mq_curmsgs;
			printf("updated\n");
		}
		
		//clear message queue part
		mq_getattr(mq2, &mq_attr2);
	    curmsg  = (int)mq_attr2.mq_curmsgs;
		while(curmsg> 0){
			printf("curmsg is:%d\n",curmsg);
			buflen2 = mq_attr2.mq_msgsize;
			bufptr2 = (char *) malloc(buflen2);

			n = mq_receive(mq2, (char *) bufptr2, buflen2, NULL);

		}
		mq_close(mq2);
		printf("All children terminated.\n");
        return 0;
	
}
