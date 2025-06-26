# 🧮 Inter-Process Histogram Generator

This project implements a concurrent client-server system using **POSIX message queues** to compute histograms from numeric data files. It demonstrates both **process-based** and **thread-based** parallelism in C.

## 🚀 Features

- Two communication models:
  - `histserver.c`: Fork-based server
  - `histserver_th.c`: Thread-based server
- POSIX message queues for inter-process/thread communication
- Dynamic histogram configuration from client input
- Efficient file parsing in parallel
- Clean shutdown and resource deallocation

## 📬 How It Works

### Client (`histclient.c` / `histclient_th.c`)

The client accepts three arguments:

- `interval_count`: number of histogram bins
- `interval_width`: width of each bin
- `start_val`: starting value of the first bin

It sends these parameters to the server via message queue and waits for histogram data in chunks.

**Example usage:**

```bash
./histclient 100 10 0
```
It prints the final histogram after aggregating received data.

### Server (histserver.c / histserver_th.c)
The server:

1. Receives histogram parameters from the client

2. Processes each file:

 - histserver.c uses child processes (fork)

 - histserver_th.c uses threads (pthread)

3. Aggregates histogram results

4. Sends results back to the client

5. Waits for a "done" message to clean up

### 🧾 Message Queue Definitions
Defined in shareddefs.h:

```bash
#define CTOSMQ "/mq1"   // Client to Server (process-based)
#define STOCMQ "/mq2"   // Server to Client (process-based)
#define CTOPMQ "/mq3"   // Children to Parent (fork-based)
#define CSTHMQ "/mq4"   // Client to Server (thread-based)
#define SCTHMQ "/mq5"   // Server to Client (thread-based)
```
### Shared Data Structures:

```bash
struct histinterval {
    int start_val;
    int end_val;
    int width;
    int count;
};

struct histdata {
    int pid;
    int histogram[501];
    int start_val;
};

struct message {
    int id;
    char text[64];
};
```
### Example Output
Final histogram counts are:
```bash
[0,10): 8
[10,20): 15
[20,30): 12
```

🛠 Compilation
Use the included Makefile to compile the project:

```bash
make
```
▶️ Execution
Fork-based version:
```bash
./histserver file1.txt file2.txt
./histclient 100 10 0
```
Thread-based version:
```bash
./histserver_th 2 data1.txt data2.txt
./histclient_th 100 10 0
```
📚 Concepts Demonstrated
- POSIX message queues

- Fork vs Thread parallelism

- File I/O and numeric parsing

- Shared data synchronization

- Queue cleanup and memory management


