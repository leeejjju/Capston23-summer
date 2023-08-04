#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> 
#include <unistd.h> 
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h> 
#include <netinet/in.h> 
#include <errno.h>

#define BUFSIZE 512
#define DEBUG
#define MODE_INPUT 0
#define MODE_OUTPUT 1
const int SUCCESS = 0;
const int FAILURE = 1;

int port = 0;
pthread_rwlock_t mutex;


typedef struct message{
	struct timeval timestamp;
	char contents[BUFSIZE];
	struct message* next;
} msg;

struct linked_list{
	msg* head;
	msg* tail;
	int size;
} llist;


// TODO how to use...?
int send_bytes(int fd, char * buf, size_t len){
    char * p = buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t sent ;
        sent = send(fd, p, len - acc, 0) ;
        if (sent == -1)
                return 1 ;
        p += sent ;
        acc += sent ;
    }
    return 0 ;
}

int recv_bytes(int fd, char * buf, size_t len){
    char * p = buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t sent ;
        sent = recv(fd, p, len - acc, 0) ;
        if (sent == -1)
                return 1 ;
        p += sent ;
        acc += sent ;
    }
    return 0 ;
}

/* compair two timeval
return 0 if a >= b, 
return 1 if a < b */
int comp(struct timeval a, struct timeval b){
	if(a.tv_sec != b.tv_sec) return (a.tv_sec < b.tv_sec);
	else return (a.tv_usec < b.tv_usec);
}

// for output client (MODE_OUTPUT)-> send messages after recv timestamp
void* sendMsgs(void* con){

	int conn = *(int*)con;
    free(con);

	//recv header(timestamp)
	struct timeval pivot;
	if(!recv(conn, &pivot, sizeof(pivot), 0)){
        perror("[cannot recv header(timestamp)]");
		goto EXIT;
    }

	int s;

	// see all the llist, send it when timestamp is larger then recv stamp
	while(1){
		// s = recv(conn, NULL, 4, 0);
		// if(s == 0) printf("> 	[OUTPUT] shutdowned\n");
		// else if(s == -1) {
		// 	printf("> 	[OUTPUT] disconnected\n");
		// 	break;
		// }else printf("> 	[OUTPUT] what the\n");

		int sendCount = 0;
		pthread_rwlock_rdlock(&mutex);
		for(msg* p = llist.head; p != NULL; p = p->next){
			if(comp(pivot, p->timestamp)){
				printf("> 	[OUTPUT] compaired %ld and %ld...\n",pivot.tv_sec, (p->timestamp).tv_sec);
				sendCount++;
				int len = strlen(p->contents);
				//send header:payloadsize
				if((s = send(conn, &len, sizeof(len), 0)) == -1){
					perror("[cannot send header(size)]");
					goto EXIT;
				}
				if(s == 0 || errno == EPIPE){
					printf("> 	[OUTPUT] client disonnected\n");
					goto EXIT;
				}
				//send header:timestamp
				if((s =send(conn, &(p->timestamp), sizeof(p->timestamp), 0)) == -1){
					perror("[cannot send header(timestamp)]");
					goto EXIT;
				}
				//send payload
				if((s = send(conn, p->contents, len, 0)) == -1){
					perror("[cannot send payload]");
					goto EXIT;
				}
				printf("> 	[OUTPUT] send: \"%s\" : %ld, savedMsg: %d\n", p->contents, p->timestamp.tv_sec, llist.size);
			}
		}
		pthread_rwlock_unlock(&mutex);
		if(sendCount != 0) {
			printf("> 	[OUTPUT] %d msg sended\n", sendCount);
			break;
		}
	}

	EXIT:
	shutdown(conn, SHUT_WR);
	printf("> [OUTPUT] socket closed --------------------------\n");
	close(conn);
	return NULL;

}


// for input client (MODE_INPUT)-> recv messages form client and save it on saved list 
void* getMsgs(void* con){
	int conn = *(int*)con;
    free(con);

	int len, s;
	int isError = 0;
	char buf[BUFSIZE];

	//recv header
	if(!recv(conn, &len, sizeof(len), 0)){
        perror("[cannot recv header]");
		isError = 1;
		goto EXIT;
    }
	
	//recv payload
	if(!(s = recv(conn, buf, len, 0))){
        perror("[cannot recv payload]");
		isError = 1;
		goto EXIT;
    }
	buf[s] = 0;

	//make the msg and save buf, add to llist
	pthread_rwlock_wrlock(&mutex);
	msg* newMsg = (msg*)malloc(sizeof(msg));
	gettimeofday(&(newMsg->timestamp), NULL);
	strcpy(newMsg->contents, buf);
	newMsg->next = NULL;

	if(llist.size == 0) {
		llist.head = newMsg;
		llist.tail = newMsg;
	}else{
		(llist.tail)->next = newMsg;
	}
	//maintain size as under 10
	if((llist.size+1) > 10){
		msg* tmp = llist.head;
		llist.head = (llist.head)->next;
		free(tmp);
	}else llist.size++;
	pthread_rwlock_unlock(&mutex);

	EXIT:
	//send header(isError)
	if(!send(conn, &isError, sizeof(isError), 0)){
        perror("[cannot send head(error)]");
    }
	printf("> 	[INPUT] recv: \"%s\" : %ld, savedMsg: %d\n", newMsg->contents, newMsg->timestamp.tv_sec, llist.size);

	shutdown(conn, SHUT_WR);
	close(conn);
	printf("> [INPUT] socket closed\n");
	return NULL;

}


int main(int argc, char** argv){

    if(argc < 2){
        printf("[server] invalid command\n");
        printf("usasg: server [port]\n");
        return EXIT_FAILURE;
    }else{
		port = atoi(argv[1]); 
    	printf("server opend -> port: %d\n", port);
	}

	llist.head = NULL;
	llist.tail = NULL;
	llist.size = 0;

	pthread_rwlock_init(&mutex, NULL);
	
	//make socket and bind
	int listen_fd, new_socket;
	struct sockaddr_in address; 
	memset(&address, '0', sizeof(address)); 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port); 
	int addrlen = sizeof(address);

	if ((listen_fd = socket(AF_INET , SOCK_STREAM, 0)) == 0)  { 
		perror("socket failed : "); 
		exit(EXIT_FAILURE); 
	}
	if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed : "); 
		exit(EXIT_FAILURE); 
	} 

	//listen for access, when accepted, make new thread for it 
	while (1) {

		if (listen(listen_fd, 20) < 0) { 
			perror("listen failed"); 
			exit(EXIT_FAILURE); 
		}
		if ((new_socket = accept(listen_fd, (struct sockaddr *) &address, (socklen_t*)&addrlen)) < 0) {
			perror("accept failed"); 
			exit(EXIT_FAILURE); 
		}
        
		//make new socket instance
        int* passing_sock  = (int*)malloc(sizeof(int));
        (*passing_sock) = new_socket;

		//get mode from header first 
		int mode;
		if(!recv(new_socket, &mode, sizeof(mode), 0)){
			perror("[cannot recv mode]"); 
			exit(EXIT_FAILURE); 
		}

		//make thread depends on mode recevd 
        pthread_t new_pid;
		if(mode == MODE_INPUT){
			printf("> [INPUT] someone is connected... %d\n", address.sin_port);
			if(pthread_create(&new_pid, NULL, (void*)getMsgs, (void*)passing_sock)){
				perror("cannot make thread");
				close(new_socket);
			}
		}else if(mode == MODE_OUTPUT){
			printf("> [OUTPUT] someone is connected... %d\n", address.sin_port);
			if(pthread_create(&new_pid, NULL, (void*)sendMsgs, (void*)passing_sock)){
				perror("cannot make new thread");
				close(new_socket);
			}
		}
	}
    
	//TODO is it meaningful..?
	pthread_rwlock_destroy(&mutex);
    return EXIT_SUCCESS;

}