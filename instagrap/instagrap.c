#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <string.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

int pipes[2];
int wport;

int checkUser(char  userinfo[30][2][30], int user_num, char * id, char * pw){
        for (int i = 0; i < user_num; i++){
                if (strcmp(userinfo[i][0], id) == 0){
                        if (strcmp(userinfo[i][1], pw) == 0)
                                return 1;
                        else
                                return 2;
                }
        }
        return 0;
}

void addUser(char  userinfo[30][2][30], int user_num, char * id, char * pw){
        strcpy(userinfo[user_num][0], id);
        strcpy(userinfo[user_num][1], pw);
}

void printUser(char  userinfo[30][2][30], int user_num){
        for (int i = 0; i < user_num; i++){
                printf("%d| id: %s, pw: %s\n", i+1, userinfo[i][0], userinfo[i][1]);
        }
}

char* out_result(char*);

char* child_proc(int conn,char *sfile_p)
{
	//get ID and PW
	char buf[1024] ;
	char * data = 0x0, * orig = 0x0, *ctemp = 0x0, *token ;
	char sid[1024] = {0}, spw[1024] = {0}, sfile[10000] = {0};
	int len = 0 ;
	int s ;
	while ( (s = recv(conn, buf, 1023, 0)) > 0 ) {
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

	// GET INFOS FROM SUBMITTER
	ctemp = data;
	token = strtok(ctemp, ":");
	strcpy(sid, token);//id
	token = strtok(0, ":");
	strcpy(spw, token);//
	token = strtok(0, "$");
	strcpy(sfile, token);
	sfile_p = sfile;
	
	// PIPE --> WRITE
	char * buff = 0x0, temp[1024];
	ssize_t ss;
	ssize_t lenn = 0;

	close(pipes[0]);
	
	sprintf(temp, "%s:%s", sid, spw);
	buff = temp;
	ss = strlen(temp);
	buff[ss] = 0x0;

	//data = buff;
	write(pipes[1], buff, ss);

	//free(buff);
	close(pipes[1]);
	
	//wait for parent process to evaluate
	return sfile_p;
}

int 
main(int argc, char  *argv[]) 
{ 
	int port, wport, c = 0, index = 0;
	int pflag = 0, wflag = 0, dflag = 0;
	char dir[1024] = {0}, wip[1024] = {0},  ctemp[1024] = {0}, *token;
	char userinfo[30][2][30];
	char sid[1024] = {0}, spw[1024] = {0};
	int user_num = 0;	

	while ( (c = getopt(argc, argv, "p:w:")) != -1){
                switch(c){
                        case 'p':
                                pflag = 1;
                                port = atoi(optarg);
                                break;
                        case 'w':
                                wflag = 1;
                                strcpy(ctemp, optarg);
                                token = strtok(ctemp, ":");
                                strcpy(wip, token);
                                token = strtok(0, ":");
                                wport = atoi(token);
                                break;
                        case '?':
                                if(optopt == 'p' || optopt == 'w')
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

                strcpy(dir, argv[index]);
                dflag = 1;

                for (index = optind + 1; index < argc; index++){
                        printf("Unnecessary argument: %s\n", argv[index]);
                }
        }

        if (pflag == 0)
                printf("Port number missing.\n");
        if (wflag == 0)
                printf("Worker port number missing.\n");
        if (dflag == 0)
                printf("Testcase directory missing.\n");

	if (pflag == 0 | wflag == 0 | dflag == 0)
		exit(1);

	printf("\n\nport: %d, wip: %s, wport: %d, dir: %s\n\n", port, wip, wport, dir);

	// LISTEN FOR SUBMITTER
	int listen_fd, new_socket ; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 

	char buffer[1024] = {0}; 

	listen_fd = socket(AF_INET /*IPv4*/, SOCK_STREAM /*TCP*/, 0 /*IP*/) ;
	if (listen_fd == 0)  { 
		perror("socket failed : "); 
		exit(EXIT_FAILURE); 
	}
	
	memset(&address, '0', sizeof(address)); 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY /* the localhost*/ ; 
	address.sin_port = htons(port); 
	if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed : "); 
		exit(EXIT_FAILURE); 
	} 

	//CREATE PIPES
        if (pipe(pipes) != 0){
                perror("PIPE ERROR\n");
                exit(1);
        }
        strcpy(buffer, "");
	
	char buff[32];
	ssize_t ss;
	int userCheck = 0;

	while (1) {
		pid_t child_pid;
		if (listen(listen_fd, 16 /* the size of waiting queue*/) < 0) { 
			perror("listen failed : "); 
			exit(EXIT_FAILURE); 
		} 

		new_socket = accept(listen_fd, (struct sockaddr *) &address, (socklen_t*)&addrlen) ;
		if (new_socket < 0) {
			perror("accept"); 
			exit(EXIT_FAILURE); 
		} 	
		child_pid = fork();
		if (child_pid == 0) {
			char sfile[1024];
			char *remain;
			remain=child_proc(new_socket,sfile) ;
			//child_proc succesfully end	
			if(remain == NULL){
				printf("Auth fail\n");
				exit(1);
			}
			sleep(3);
			// Make connection with worker
			struct sockaddr_in serv_addr;
       			int sock_fd ;
        		int s, len ;
        		char * data;

        		sock_fd = socket(AF_INET, SOCK_STREAM, 0) ;
        		if (sock_fd <= 0) {
                		perror("socket failed : ") ;
                		exit(EXIT_FAILURE) ;
        		}

        		memset(&serv_addr, '0', sizeof(serv_addr));
        		serv_addr.sin_family = AF_INET;
        		serv_addr.sin_port = htons(wport);
        		if (inet_pton(AF_INET, wip, &serv_addr.sin_addr) <= 0) {
                		perror("inet_pton failed : ") ;
                		exit(EXIT_FAILURE) ;
        		}
        		if (connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
                		perror("connect failed : ") ;
                		exit(EXIT_FAILURE) ;
        		}

			printf("send in file to worker \n");
			char result[1024],output[1024],tmp[1024];
			int i;
			for(i=0;i<10;i++){
	                        char in_file[6]=" .in";
        	                FILE* fptr;
                	        char num;
				output[0]='\0';
				tmp[0]='\0';
	              	    //    char output[1024],tmp[100];
	                        if(i!=9)num = '1'+i;
	                        if(i!=9)in_file[0]=num;
	                        else strcpy(in_file,"10.in");
	                        fptr=fopen(in_file,"r");

	                        while( fgets(tmp,sizeof(tmp),fptr) != NULL){
	                                if(output[0]=='\0'){
	                                         strcpy(output,tmp);
	                                }
	                                else strcat(output,tmp);
					if(output[strlen(output)-1]=='\n')output[strlen(output)-1]='\0';
         	               }
                       	strcat(result,output);
                        strcat(result,"::");
                	}
			printf("\nsend in file as string %s\n",result);
			data = result ;
                        len = strlen(result) ;
                        s = 0 ;
                        while (len > 0 && (s = send(sock_fd, data, len, 0)) > 0) {
                                data += s ;
                                len -= s ;
                        }

			printf("send C file to worker : %s\n",remain);
                        data = remain ;
                        len = strlen(remain) ;
                        s = 0 ;
                        while (len > 0 && (s = send(sock_fd, data, len, 0)) > 0) {
                                data += s ;
                                len -= s ;
                        }

			sleep(2);

			printf("Send complete waiting for result\n");	
			shutdown(sock_fd,SHUT_WR);
			char buf[1024] ;
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
        		printf(">%s\n", data);
        		}
			char *send_result;
			if(strcmp(data,"execution fail::execution fail::execution fail::execution fail::execution fail::execution fail::execution fail::execution fail::execution fail::execution fail::")==0){
			char reresult[100]="\nBuild fail\n";
			send_result = reresult;
        }

			else {
				send_result = out_result(data);
			}
			printf("%s\n",send_result);
                        len = strlen(send_result) ;
                        s = 0 ;
                        while (len > 0 && (s = send(new_socket, send_result, len, 0)) > 0) {
                                send_result += s ;
                                len -= s ;
                        }
			shutdown(sock_fd, SHUT_WR);
			shutdown(new_socket, SHUT_WR);
			
		} else {
			//wait until ID and PW is written
			printf("waiting for user info from child\n");
                        ss = read(pipes[0], buff, 14);
			printf("get user info from child\n");
                        buff[ss] = 0x0;
			printf(">>>%s\n", buff);

			strcpy(ctemp, buff);
       			token = strtok(ctemp, ":");
        		strcpy(sid, token);
        		token = strtok(0, ":");
        		strcpy(spw, token);

			userCheck = checkUser(userinfo, user_num, sid, spw);
			printf("userCheck: %d\n", userCheck);

			if (userCheck == 0){
				printf("Adding new user\n");
				addUser(userinfo, user_num, sid, spw);
				user_num++;
				wait(NULL);
			} else if (userCheck == 1){
				printf("Accepted user\n");
				wait(NULL);
			} else if (userCheck == 2){
				printf("Wrong password\n");
				kill(child_pid,SIGINT);
			}
			printf("Child Process is done!\n");


			close(pipes[1]);
			close(new_socket) ;
			sleep(10);


		}
	}


}

char* out_result(char* eval_data){
        int result[11],i;//0: success 1: fail 2: Timeout
        FILE *fptr;
        char *eval;
        char num;
        char out_file[6]=" .out";
        char output[1024], tmp[100];
        eval = strtok(eval_data,"::");
        for(i=0;i<10;i++){
                output[0]='\0';
                tmp[0]='\0';

                //tokenize
                //open i.out
                if(i!=9)num = '1'+i;
                if(i!=9)out_file[0]=num;
                else strcpy(out_file,"10.out");
                fptr = fopen(out_file,"r");
                while( fgets(tmp,sizeof(tmp),fptr) != NULL){
                        if(output[0]=='\0'){
                                strcpy(output,tmp);
                        }
                        else strcat(output,tmp);
                        if(output[strlen(output)-1]=='\n')output[strlen(output)-1]='\0';
                }
                //evaluate results
                if(strcmp(eval,"Timeout")==0){
                        result[i]=2;
                }
                else if(strcmp(eval,output)==0){
                        result[i]=0;
                }
                else {
                        result[i]=1;
                }
                fclose(fptr);
                eval = strtok(NULL,"::");
        }
	char result_for_transfer[1024]="\nResult of your code\n";
        for(i=0;i<10;i++){
		char num2[]="Input  :  ";
		if(i!=9)num2[7]='1'+i;
		else {
			num2[7]='1';
			num2[8]='0';
		}
		strcat(result_for_transfer,num2);
                if(result[i] == 0){
                        strcat(result_for_transfer,"Correct!\n");
                }
                else if(result[i] == 2){
			strcat(result_for_transfer,"Timeout!\n");
                }
                else strcat(result_for_transfer,"Incorrect!\n");

        }
	eval_data = result_for_transfer;
	return eval_data;
}

