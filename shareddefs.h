
struct histinterval{
	int start_val;
	int end_val;
	int width;
	int count;
};
struct histdata{
	int pid;
	int histogram[301]; 
	int start_val;
};
struct message {
        int id;
        char text[64];
};


#define CTOSMQ "/mq1"
#define STOCMQ "/mq2"
#define CTOPMQ "/mq3"
