#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h> 
#include <signal.h>
#include <sys/time.h>

int flag = 0;
void
handler(int sig){
        printf("Waiting for result..\n");
        flag = 0;
}


int 
main(int argc, char *argv[]) 
{
	int c, index, port;
	int ipflag = 0, idflag = 0, pwflag = 0, fflag = 0;
	char ip[1024] = {0}, id[1024] = {0}, pw[1024] = {0}, file[1024] = {0}, ctemp[1024] = {0}, *token;

	while ((c = getopt(argc, argv, "n:u:k:")) != -1){
		switch(c){
			case 'n':
				ipflag = 1;
				strcpy(ctemp, optarg);
				token = strtok(ctemp, ":");
				strcpy(ip, token);
				token = strtok(0, ":");
				port = atoi(token);
				printf("ip: %s, port: %d\n", ip, port);
				break;
			case 'u':
				idflag = 1;
				strcpy(id, optarg);
				printf("id: %s\n", id);
				break;
			case 'k':
				pwflag = 1;
				strcpy(pw, optarg);
				printf("pw: %s\n", pw);
				break;
			case '?':
				if (optopt == 'n' || optopt == 'u' || optopt == 'k')
					fprintf(stderr, "Option -%c needs argument.\n", optopt);
				else 
					fprintf(stderr, "Unknown option -%c.\n", optopt);
				break;
			default:
				fprintf(stderr, "getopt");			
		}
	}

	index = optind;
	if (index < argc){
		fflag = 1;
		strcpy(file, argv[index]);
		printf("file: %s\n", file);

		for (index = optind + 1; index < argc; index++){
			printf("Unnexessary argument: %s\n", argv[index]);
		}
	}

	if (ipflag == 0)
		printf("IP and port number missing.\n");
	if (idflag == 0)
		printf("Student ID missing.\n");
	if (pwflag == 0)
		printf("Student password missing.\n");
	if (fflag == 0)
		printf("Target file name missing.\n");
	
	if (ipflag == 0 | idflag == 0 | pwflag == 0| fflag == 0)
		exit(1);

	struct sockaddr_in serv_addr; 
	int sock_fd ;
	int s, len ;
	char buffer[1024] = {0}; 
	char * data ;
	
	sock_fd = socket(AF_INET, SOCK_STREAM, 0) ;
	if (sock_fd <= 0) {
		perror("socket failed : ") ;
		exit(EXIT_FAILURE) ;
	} 

	memset(&serv_addr, '0', sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(port); 
	if (inet_pton(AF_INET,ip , &serv_addr.sin_addr) <= 0) {
		perror("inet_pton failed : ") ; 
		exit(EXIT_FAILURE) ;
	} 

	if (connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("connect failed : ") ;
		exit(EXIT_FAILURE) ;
	}
	// send userinfo
	
//	sprintf(buffer, "%s:%s:", id, pw, file);
	sprintf(buffer, "%s:%s:", id, pw);
	data = buffer;
	len = strlen(buffer) ;
	s = 0 ;
	while (len > 0 && (s = send(sock_fd, data, len, 0)) > 0) {
		data += s ;
		len -= s ;
	}
	//Sending File to instagrap
	char file_name[1024];
        FILE *fp;
        strcpy(file_name,file);
        fp=fopen(file_name,"r");
        file_name[strlen(file_name)]='\n';
        printf("File name : %s\n",file_name);
        strcpy(buffer,file_name);
        do{
                data = buffer ;
                len = strlen(buffer) ;
                s = 0 ;
                while (len > 0 && (s = send(sock_fd, data, len, 0)) > 0) {
                        data += s ;
                        len -= s ;
                }
        }while(fgets(buffer,1024,fp));
	
        
	shutdown(sock_fd, SHUT_WR) ;
	// send file with file name

	// Using sigalarm and waiting for result
        char buf[1024] ;
        struct itimerval t;
        signal(SIGALRM, handler);
        t.it_value.tv_sec = 2;
        t.it_interval = t.it_value;
        setitimer(ITIMER_REAL, &t ,0x0);

        while(1){
                if(flag == 0){
                        data = 0x0 ;
                        len = 0 ;
                        while ( (s = recv(sock_fd, buf, 1023, 0)) > 0 ) {
                                buf[s] = 0x0 ;
                                if (data == 0x0) {
                                        data = strdup(buf) ;
                                        len = s ;
                                }
                                else {
                                        data = realloc(data, len + s + 1) ;
                                        strncpy(data + len, buf, s) ;
                                        data[len + s] = 0x0 ;
                                        len += s ;
                                }
                        }
                        if(data != NULL){
                                printf("Successfully get results %s\n",data);
                                shutdown(sock_fd, SHUT_WR) ;
                                exit(0);
                        }
                        else printf("%s\n", data);
                        printf("wait\n");
                        flag = 1;
                }
        }	
} 
