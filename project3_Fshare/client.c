#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#define BUFSIZE 4096



int main(int argc, char** argv){

    if(argc < 3){
        goto EXIT;
    }
    char* ip = (char*)malloc(sizeof(char)*strlen(argv[1])+1);
    char* cmd = (char*)malloc(sizeof(char)*strlen(argv[2])+1);
    strcpy(ip, argv[1]);
    strcpy(cmd, argv[2]);

    //get ip and port
    int index = 0;
    int port = 0;

    for(int i = 0; i < strlen(ip); i++){
        if(ip[i] == ':') index = i;
    }

    ip[index] = 0;
    port = atoi(&ip[index+1]);

    printf("ip: %s\nport: %d\n", ip, port);


    struct sockaddr_in serv_addr; 
	int sock_fd ;
	int s, len ;
	char buffer[BUFSIZE] = {0}; 
	char * data ;
	
	sock_fd = socket(AF_INET, SOCK_STREAM, 0) ;
	if (sock_fd <= 0) {
		perror("socket failed : ") ;
		exit(EXIT_FAILURE) ;
	} 

	memset(&serv_addr, '\0', sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(port); 
	if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
		perror("inet_pton failed : ") ; 
		exit(EXIT_FAILURE) ;
	} 

	if (connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("connect failed : ") ;
		exit(EXIT_FAILURE) ;
	}

	// scanf("%s", buffer) ;
    sprintf(buffer,"%s %s", argv[2], argv[3]);
	
	data = buffer ;
	len = strlen(buffer) ;
	s = 0 ;
	while (len > 0 && (s = send(sock_fd, data, len, 0)) > 0) {
		data += s ;
		len -= s ;
	}

	shutdown(sock_fd, SHUT_WR) ;
    printf("\n");

	char buf[BUFSIZE] ;
	data = 0x0 ;
	len = 0 ;
	while ( (s = recv(sock_fd, buf, BUFSIZE-1, 0)) > 0 ) {
        if(s == 0){
            perror("[cannot read contents]");
            free(ip);
            free(cmd);
            return EXIT_FAILURE;
        }
		buf[s] = 0x0 ;

        printf("%s", buf);

		// if (data == 0x0) {
		// 	data = strdup(buf) ;
		// 	len = s ;
		// }
		// else {
		// 	data = realloc(data, len + s + 1) ;
		// 	strncpy(data + len, buf, s) ;
		// 	data[len + s] = 0x0 ;
		// 	len += s ;
		// }
        // printf(">%s", data); 
	}
    printf("\n");
	
	

    // if(!strcmp(cmd, "list")){
    //     char buf[BUFSIZE] ;
    
    //     //write to server 
    //     sprintf(buf, "%s %s", argv[2], "");
    //     data = buf;
    //     len = strlen(data);
    //     s = 0 ;

    //     while (len > 0 && (s = send(sock_fd, data, len, MSG_DONTWAIT)) > 0) {
    //         data += s ;
    //         len -= s ;
    //     }
    //     shutdown(sock_fd, SHUT_WR) ;

    //     //read from server
        
    //     data = 0x0 ;
    //     len = 0 ;
    //     while ( (s = recv(sock_fd, buf, BUFSIZE, 0)) > 0 ) {
    //         buf[s] = 0x0 ;
    //         if (data == 0x0) {
    //             data = strdup(buf) ;
    //             len = s ;
    //         }
    //         else {
    //             data = realloc(data, len + s + 1) ;
    //             strncpy(data + len, buf, s) ;
    //             data[len + s] = 0x0 ;
    //             len += s ;
    //         }

    //     }
    //     printf("> %s\n", data); 


    // }else if(!strcmp(cmd, "get")){
    //     if(argc < 4){
    //         goto EXIT;
    //     }
    //     //make new file and open it 
    //     //send request to server
    //     //receive stream and write to the file
    //     //close file
    // }else if(!strcmp(cmd, "put")){
    //     if(argc < 4){
    //         goto EXIT;
    //     }
    //     //open file
    //     //send request to server
    //     //send contents to server 
    //     //close file 
    // }else{
        EXIT:

        // printf("[client] invalid command\n");
        // printf("usasg: \n");
        // if(argc > 2){
        //     free(ip);
        //     free(cmd);
        // }
        // return EXIT_FAILURE;
    // }

    printf("done!\n");
    free(ip);
    free(cmd);
    return EXIT_SUCCESS;

}
