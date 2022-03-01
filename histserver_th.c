
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <time.h>

#include "shareddefs.h"
#define MAX_LINE_LENGTH 300

#define MAXTHREADS  10		/* max number of threads */
#define MAXFILENAME 10		/* max length of a filename */

struct arg {
    char *afileName;
	int t_index;		/* the index of the created thread */
};
int intervalcount;
struct histinterval* histogram;


/* this is function to be executed by the threads */
static void *threadfunc(void *arg_ptr)
{
    FILE *newFile = fopen(((struct arg *) arg_ptr)->afileName, "r");
	//char *retreason; 

	printf("thread %d started \n", ((struct arg *) arg_ptr)->t_index);
	
	if (newFile == NULL) {
		perror("error in thread func");
		exit(1);
	}

    
    char buffer[MAX_LINE_LENGTH];
     while (fgets(buffer, MAX_LINE_LENGTH, newFile)){
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
        fclose(newFile);
    //send by using message queue
 

	pthread_exit(NULL);  // just tell a reason to the thread that is waiting in join
}




int main(int argc, char *argv[])
{
    //MESSAGE QUEUE BETWEEN SERVER AND CLIENT
    //variables
    mqd_t mq, mq3;
    struct mq_attr mq_attr, mq_attr3;
    struct message *messageptr;
    struct message *messageptr2;
    int n;
    char *bufptr;
    char *bufptr3;
    char *bufptr4;
    int buflen,buflen4,buflen3;
    //struct histdata *histdataptr;
    int  intervalwidth, intervalstart;
    ////recieve message from the client
     mq = mq_open(CSTHMQ, O_RDWR | O_CREAT, 0666, NULL);
    

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
    histogram = malloc(sizeof(struct histinterval)* intervalcount);
    int j;
  
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
    
    
    ///THREADs
	pthread_t tids[MAXTHREADS];	/*thread ids*/
	int count;		        /*number of threads*/
	struct arg t_args[MAXTHREADS];	/*thread function arguments*/
	
	int i;
	int ret;
	char *retmsg; 

	if (argc < 2|| argc > 12 ) {
		printf
		    ("not right file count\n");
		exit(1);
	}



    int noOfFile = atoi(argv[1]);
    count = atoi(argv[1]);
   /* char* fileArr[noOfFile];
    for (int i = 0; i < noOfFile; i++){
      
      fileArr[i] = argv[i + 2];
      
    }*/
	for (i = 0; i < noOfFile; ++i) {
        t_args[i].afileName = argv[i + 2];
		t_args[i].t_index = i;
      //printf("thread create failed %s \n",t_args[i].afileName);
      
		ret = pthread_create(&(tids[i]),
				     NULL, threadfunc, (void *) &(t_args[i]));

		if (ret != 0) {
			printf("thread create failed \n");
			exit(1);
		}
		printf("thread %i with tid %u created\n", i,
		       (unsigned int) tids[i]);
	}


	printf("main: waiting all threads to terminate\n");
	for (i = 0; i < count; ++i) {
	    ret = pthread_join(tids[i], (void **)&retmsg);
		if (ret != 0) {
			printf("thread join failed \n");
			exit(1);
		}
		printf ("thread terminated\n");
		// we got the reason as the string pointed by retmsg
		// space for that was allocated in thread function; now freeing. 
		free (retmsg); 
	}

	printf("main: all threads terminated\n");
    
    
    
    
    //////////////THREADS
    
    mq3 = mq_open(SCTHMQ, O_RDWR| O_CREAT, 0666, NULL);
    if (mq3 == -1) {
        perror("can not open server to client msg queue\n");
        exit(1);
    }
    printf("server to client mq opened, mq id = %d\n", (int) mq);
    
    int l;
    for(l = 0 ; l < iterationtime; l++){
            
            struct histdata histdatath;
            histdatath.pid = getpid();
            
            if(l < iterationtime -1 ){
                //replace full to the array
                int t;
                for(t = 0 ; t < 501; t++){
                    histdatath.histogram[t]= histogram[501*l+t].count;
                }
                histdatath.start_val = histogram[501*l].start_val;
            }else if(l == iterationtime -1){
                //find modular of 501
                int mod = intervalcount % 501;
                int t;
                if(mod == 0){
                    histdatath.start_val = -1;
                }
                else{
                    histdatath.start_val = histogram[501*l].start_val;
                }
                for(t = 0 ; t < mod; t++){
                    histdatath.histogram[t] = histogram[501*l+t].count;
                }
                histdatath.histogram[mod] = -1;
                for(t = mod +1 ; t < 1; t++){
                    histdatath.histogram[mod] = -2;
                }
            }
    
            //print histdata
            
            //look at the message count in the queue
            mq_getattr(mq3, &mq_attr3);
            //send histdata to the client
            n = mq_send(mq3, (char*) &histdatath,sizeof(struct histdata),0);
            if (n == -1) {
                perror("server_th to client_th mq_send failed\n");
                exit(1);
            }
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
            free(histogram);
            mq_close(mq);
            mq_close(mq3);
            printf("server securely closed\n");
            return 1;
        }
        printf("server cannot securely closed");
        return -1;
        

}



   
            
        
              