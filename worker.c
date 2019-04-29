#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>

//Functions for signal 
void
handler(int sig)
{       
        if (sig == SIGINT) {
                exit(0) ;
        }
}
void *timer( void *data){
        pid_t *pid = (pid_t *)data;
        sleep(3);       
        if(kill(*pid,SIGINT)==0){
                printf("Timeout\n");
        }
}
//make connection with instagrap
void
child_proc(int conn)
{
        char buf[1024] ;
        char * data = 0x0, * orig = 0x0 ;
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
	//get file and separate file name
	int i;
        char file_name[1024];
        FILE *fp;
	
        printf(">%s\n", data) ;
       	//Create in file
	char *eval;
	char infile_data[1024];
	strcpy(infile_data,data);
	eval = strtok(infile_data,"::");
	for(i=0;i<10;i++){
		printf("%s\n",eval);
                char in_file[6]=" .in";
                FILE* fptr;
                char num;
                if(i!=9)num = '1'+i;
                if(i!=9)in_file[0]=num;
                else strcpy(in_file,"10.in");
                fptr=fopen(in_file,"w");
	        fprintf(fptr,"%s\n",eval);
		eval = strtok(0,"::");
		fclose(fptr);
        }
	int loc=0;
	for(i=strlen(data);i>=0;i--){
		if(data[i] == ':' && data[i-1] == ':'){
			loc = i;
			break;
		}
	}
	printf("%s\n",data);	
	char handle_file_name[1024];	
	for(i=0;i<strlen(data);i++){
                if(data[i]=='.' && data[i+1]=='c'){
                        strncpy(handle_file_name,data,i+2);
                        handle_file_name[i+2]='\0';
                        break;
                }
        }
	for(i=loc+1;i<strlen(handle_file_name);i++){
		file_name[i-(loc+1)] = handle_file_name[i];
	}
        printf("%s\n",file_name);
        fp=fopen(file_name,"w");
        for(i=(loc+1)+strlen(file_name)+1;i<strlen(data);i++){
                fprintf(fp,"%c",data[i]);
        }
        fclose(fp);
	
	//evaluate process init
        pid_t child_pid;
        pid_t eval_pid;
        char str[1024];
        char *compile_name;
	int buildfailcheck=0;
	char result[1024];
        //compile file with filename without '.c'
        strcpy(str,file_name);
        compile_name = strtok(str,".");
        child_pid=fork();
        if(child_pid==0){
                if(execl("/usr/bin/gcc","gcc","-o",compile_name,file_name,NULL)==-1){
                        strcpy(result,"buildfail");
			printf("build fail");
			buildfailcheck=1;
                        exit(0);
                }
        }
        wait(NULL);
	if(buildfailcheck != 1){
        	for(i=0;i<10;i++){
	    	    eval_pid=fork();
        	    if(eval_pid==0){
                	char path[1024]="./";
                  	char num;
                        char in_file[6]=" .in";
                        char out_file[6]=" .out";
                        FILE* fptr;
                        pid_t child_child_pid;
                        pthread_t timer_thread;
                        if(i!=9)num = '1'+i;
                        if(i!=9){
                                in_file[0]=num;
                                out_file[0]=num;
                        }
                        else {
                                strcpy(in_file,"10.in");
                                strcpy(out_file,"10.out");
                        }
                        strcat(path,compile_name);
                        printf("processing : %s %s %s\n",in_file,out_file,path);
                   //     printf("%s %s\n",path,compile_name);
                        child_child_pid = fork();
                        pthread_create(&timer_thread, NULL, timer, &child_child_pid);
                        int fd_output = open(out_file, O_WRONLY | O_CREAT, 0644) ;
                        dup2(fd_output , 1);
                        if(child_child_pid==0){
                                int fd_output = open(out_file, O_WRONLY | O_CREAT, 0644) ;
                                int fd_input = open(in_file, O_RDONLY);
                                dup2(fd_output , 1);
                                dup2(fd_input , 0);
                                if(execl(path,compile_name,NULL) == -1){
                                         printf("execution fail");
                                         exit(0);
                                }
                                close(fd_output);
                                close(fd_input);
                        }
                        wait(NULL);
			close(fd_output);
                        exit(0);
               	   }
                   //else wait(NULL);
       	      }	
		//make .out files into 1 result stirng
	//data[0]='\0';
		result[0]='\0';
		for(i=0;i<10;i++){
			char out_file[6]=" .out";
             	 	FILE* fptr;
			char num;
			char output[1024],tmp[100];
                	if(i!=9)num = '1'+i;
                	if(i!=9)out_file[0]=num;
                	else strcpy(out_file,"10.out");
                	fptr=fopen(out_file,"r");
		
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
		printf("send result : %s",result);
	}
	data = result;
//	orig=data;
	while (len > 0 && (s = send(conn, data, len, 0)) > 0) {
                data += s ;
                len -= s ;
        }
        shutdown(conn, SHUT_WR) ;
  //      if (orig != 0x0)
    //            free(orig) ;
       // exit(0);
}
// execute_function
void *execute_function( void *ptr )
{
        pthread_t tid;
        tid = pthread_self();
        char *message;
        message = (char *) ptr;
        printf("PID: %d - %s \n",(int)tid, message);
        //execl("./","gcc","-o",compile_name,file_name,NULL)
}

int
main(int argc, char *argv[])
{
	int c, index, port, pflag = 0;
	//getopt for worker.c
	while((c = getopt(argc, argv, "p:")) != -1){
		switch(c){
			case 'p':
				pflag = 1;
				port = atoi(optarg);
				printf("port: %d\n", port);
				break;
			case '?':
				if (optopt == 'p')
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
		for (index = optind + 1; index < argc; index++){
                        printf("Unnexessary argument: %s\n", argv[index]);
                }
	}

	if (pflag == 0)
                printf("Port number missing.\n");
	

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

        while (1) {
                if (listen(listen_fd, 16 /* the size of waiting queue*/) < 0) {
                        perror("listen failed : ");
                        exit(EXIT_FAILURE);
                }

                new_socket = accept(listen_fd, (struct sockaddr *) &address, (socklen_t*)&addrlen) ;
                if (new_socket < 0) {
                        perror("accept");
                        exit(EXIT_FAILURE);
                }

                if (fork() > 0) {
                        child_proc(new_socket) ;
			system("make clean");
                }
                else {
                        close(new_socket) ;
                }
        }
}
