/* header files */
#include	<stdio.h>
#include	<stdlib.h>
#include	<netdb.h>	/* getservbyname(), gethostbyname() */
#include	<errno.h>	/* for definition of errno */
#include    <sys/socket.h>
#include    <sys/types.h>
#include    <arpa/inet.h>
#include    <sys/stat.h>
#include    <sys/time.h>
#include    <assert.h>
#include    <string.h>
#include    <unistd.h>
#include    <termios.h>
#include    <fcntl.h>
#define ECHOFLAGS (ECHO | ECHOE | ECHOK | ECHONL) 
#define MAXBUF	1024
#define STDIN_FILENO	0
#define STDOUT_FILENO	1

/* define FTP reply code */
#define USERNAME	220
#define PASSWORD	331
#define LOGIN		230
#define PATHNAME	257
#define CLOSEDATA	226
#define ACTIONOK	250
#define TYPEINFORMATION 200

/* DefinE global variables */
char	*host;		/* hostname or dotted-decimal string */
int	 port;
char	*rbuf, *rbuf1;		/* pointer that is malloc'ed */
char	*wbuf, *wbuf1;		/* pointer that is malloc'ed */
struct sockaddr_in	servaddr, local_host;
static   char   re_addr[100];
static   char   lo_addr[100];
static   char   cd_addr[100];
int   ls = 0;
int   dir = 0;
int   get = 0;
int   put = 0;
int   mode = 0;
int   dataType=0;


int	cliopen(char *host, int port);
int portopen(int sockfd);
int portacc(int tran_sock0);
int	strtosrv(char *str);
void	cmd_tcp(int sockfd);
void	ftp_list(int sockfd);
int	ftp_get(int sck, char *pDownloadFileName_s);
int	ftp_put (int sck, char *pUploadFileName_s);
int set_disp_mode(int fd,int option);
int getpasswd(char* passwd,int size);

int set_disp_mode(int fd,int option)
{
    int err;
    struct termios term;
    if(tcgetattr(fd,&term)==-1){
        perror("Cannot get the attribution of the terminal");
        return 1;
    }
    if(option)
        term.c_lflag|=ECHOFLAGS;
    else
        term.c_lflag &=~ECHOFLAGS;
    err=tcsetattr(fd,TCSAFLUSH,&term);
    if(err==-1 && err==EINTR){
        perror("Cannot set the attribution of the terminal");
        return 1;
    }
    return 0;
}

int getpasswd(char* passwd, int size)
{
    int c;
    int n = 0;
    
    
    do{
        c=getchar();
        if (c != '\n'|c!='\r'){
            passwd[n++] = c;
        }
    }while(c != '\n' && c !='\r' && n < (size - 1));
    passwd[n] = '\0';
    return n;
}


int
main(int argc, char *argv[])
{
	int	fd;

	if (0 != argc-2)
	{
		printf("%s\n","missing <hostname>");
		exit(0);
	}

	host = argv[1];
	port = 21;

	/*****************************************************************
	//1. code here: Allocate the read and write buffers before open().
	*****************************************************************/
    rbuf = (char *)malloc(MAXBUF*sizeof(char));
    rbuf1 = (char *)malloc(MAXBUF*sizeof(char));
    wbuf = (char *)malloc(MAXBUF*sizeof(char));
    wbuf1 = (char *)malloc(MAXBUF*sizeof(char));
	fd = cliopen(host, port);

	cmd_tcp(fd);

	exit(0);
}


/* Establish a TCP connection from client to server */
int
cliopen(char *host, int port)
{
	/*************************************************************
	//2. code here 
	*************************************************************/
    int sockfd;
    struct hostent *hostt=NULL;
    struct in_addr addr;
    printf("host_ip is %s\n",host);
   // if (inet_aton(host,&addr)>=0) {
     // hostt=gethostbyaddr(&addr,4,AF_INET);
      //  printf("the host ip addr is %s\n",inet_ntoa(*((struct in_addr *)hostt->h_addr)));
      //  printf("111\n");
    //}else{
       hostt=gethostbyname(host);
       // printf("222\n");
    //}
    if(hostt==NULL){
        printf("You enter a wrong address information");
    }
    memset(&servaddr,0,sizeof(struct sockaddr_in));
    memcpy(&servaddr.sin_addr.s_addr,hostt->h_addr,hostt->h_length);
    servaddr.sin_family=AF_INET;//USE TCP-IP
    servaddr.sin_port=htons(port);//use port 21
        //servaddr.sin_addr=*((struct in_addr*)hostt->h_addr);//server address
    if ((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1) {
        perror("socket failed!\n");
        exit(1);
        
    }
    printf("get socket");
  
    if (connect(sockfd,(struct sockaddr *)&servaddr,sizeof(struct sockaddr))==-1) {
        printf("!error happens when connrcting to server!\n");
        exit(1);
        
    }
    printf("connect done!\n");

    //printf("client_port is %d\n",ntohs(client_addr.sin_port));
    return sockfd;
    
}

/*
   Open port for active mode
*/
int
portopen(int sockfd){
        int tran_port, tran_sock;
		struct sockaddr_in local;
		char *local_ip;
		int add[6];
		int addr_len =  sizeof(struct sockaddr);
        srand((unsigned)time(NULL));
		tran_port = rand() % 40000 + 1025;
		if((tran_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
             perror("socket failed!\n");
             exit(1);
		}
		bzero(&local_host,sizeof(local_host));
		local_host.sin_family = AF_INET;
		local_host.sin_port = htons(tran_port);
		local_host.sin_addr.s_addr = htonl(INADDR_ANY);
		bzero(&local, sizeof(struct sockaddr));
		while(1)
		{
			if(bind(tran_sock, (struct sockaddr *)&local_host, 
					sizeof(local_host)))
			{
				srand((unsigned)time(NULL));
				tran_port = rand() % 40000 + 1025;
				continue;
			}
			if(listen(tran_sock, 10) != 0)
			{
				perror("listen");
                exit(1);
			}
			if(getsockname(sockfd,(struct sockaddr*)&local,
                               (socklen_t *)&addr_len) < 0)
				return -1;
			local_ip = inet_ntoa(local.sin_addr);
			//printf("%s\n",local_ip);
			sscanf(local_ip,"%d.%d.%d.%d",&add[0],&add[1],&add[2],&add[3]);
			add[5]=tran_port%256;
			add[4]=(tran_port-add[5])/256;
			sprintf(wbuf, "PORT %d,%d,%d,%d,%d,%d\n",add[0],add[1],add[2],add[3],add[4],add[5]);
			//printf("%s",wbuf);
			return tran_sock;
		}
	}
int portacc(int tran_sock0){
     int sin_size = sizeof(struct sockaddr_in);
     int tran_sock;
     while(1) {
       if((tran_sock=(tran_sock0,(struct sockaddr *)&servaddr,&sin_size))==-1){
          perror("accept");
          exit(1);}
       else{
          printf("Accept successfully\n");
          break;
        }
        }
    return tran_sock;
}


/*
   Compute server's port by a pair of integers and store it in char *port
   Get server's IP address and store it in char *host
*/
int
strtosrv(char *str)
{
	int add[6];
    sscanf(str,"%*[^(](%d,%d,%d,%d,%d,%d)",&add[0],&add[1],&add[2],&add[3],&add[4],&add[5]);
    bzero(host,strlen(host));
    printf("%d.%d.%d.%d",add[0],add[1],add[2],add[3]);
    sprintf(host,"%d.%d.%d.%d",add[0],add[1],add[2],add[3]);
    return add[4]*256+add[5];
     
}


/* Read and write as command connection */
void
cmd_tcp(int sockfd)
{
	int		maxfdp1, nread, nwrite, fd, replycode,i,tran_sock,tran_sock0;
	fd_set		rset;

	FD_ZERO(&rset);
	maxfdp1 = sockfd + 1;	/* check descriptors [0..sockfd] */

	for ( ; ; )
	{
        //printf("enter for cycle");
		FD_SET(STDIN_FILENO, &rset);
		FD_SET(sockfd, &rset);
        if(replycode==PASSWORD) {
			char passwd[20];
            set_disp_mode(STDIN_FILENO,0);
            getpasswd(passwd, sizeof(passwd));
            set_disp_mode(STDIN_FILENO,1);
            printf("\n");
            sprintf(wbuf,"PASS %s\n",passwd);
            if (write(sockfd, wbuf, strlen(wbuf)) != strlen(wbuf))
                printf("write error\n");
        }

		if(select(maxfdp1, &rset, NULL, NULL, NULL) < 0)
			printf("select error\n");
        
		/* data to read on stdin */
      		if(FD_ISSET(STDIN_FILENO, &rset)) {
            nwrite=0;
            nread=0;
             bzero(wbuf,MAXBUF);
             bzero(rbuf,MAXBUF);
            
            if( (nread = read(STDIN_FILENO, rbuf, MAXBUF)) < 0){
				printf("read error from stdin\n");}
                nwrite = nread+5;

			/* send username */
			if(replycode == USERNAME) {
				sprintf(wbuf, "USER %s", rbuf);
				if(write(sockfd, wbuf, nwrite) != nwrite)
					printf("write error\n");
			}

			 /*************************************************************
			 //4. code here: send password
			 *************************************************************/
            
			 /* send command */
			if(replycode == 550 ||replycode==LOGIN || replycode==CLOSEDATA || replycode==PATHNAME || replycode==ACTIONOK||replycode==TYPEINFORMATION)
			{
				/* ls - list files and directories*/
				if(strncmp(rbuf, "ls", 2) == 0) {
					if(mode==0){
					    sprintf(wbuf, "%s", "PASV\n");
                        nwrite = 5;
                        if( ( write(sockfd, wbuf, 5) ) != nwrite){
                        printf("write error\n");
                        }
					}
					else{
						tran_sock0 = portopen(sockfd);
                        write(sockfd, wbuf, strlen(wbuf));
					}
                    ls = 1;
                    continue;
				}

				/*************************************************************
				// 5. code here: cd - change working directory/
				*************************************************************/
                if(strncmp(rbuf,"cd",2)==0) {
                    sscanf(rbuf,"cd %s",cd_addr);
                    sprintf(wbuf,"CWD %s\n",cd_addr);
                    if(write(sockfd, wbuf, strlen(wbuf)) != strlen(wbuf))
                        printf("write error\n");
                    continue;
                }
                if(strncmp(rbuf,"binary",6)==0) {
                    sprintf(wbuf,"%s","TYPE I\n");
                    if(write(sockfd, wbuf,7) != 7)
                        printf("write error\n");
                    dataType=0;
                    continue;
                }
                if(strncmp(rbuf,"ascii",5)==0) {
                    sprintf(wbuf,"%s","TYPE A\n");
                    if(write(sockfd, wbuf,7) != 7)
                        printf("write error\n");
                    dataType=1;
                    continue;
                }
                if(strncmp(rbuf,"type",4)==0) {
                    if (dataType==0) {
                        printf("You are using binary mode.\n");
                    } else {
                        printf("You are using ascii mode.\n");
                    }
                    continue;
                }




				/* pwd -  print working directory */
				if(strncmp(rbuf, "pwd", 3) == 0) {
					sprintf(wbuf, "%s", "PWD\n");
					write(sockfd, wbuf, 4);
					continue;
				}

				/*************************************************************
				// 6. code here: quit - quit from ftp server
				*************************************************************/
                if(strncmp(rbuf,"exit",4)==0) {
                    sprintf(wbuf,"%s","QUIt\r\n");
                    if (write(sockfd,wbuf,6)!=nwrite) {
                        printf("write error\n");
                    }
                    continue;
                }

				/*************************************************************
				// 7. code here: get - get file from ftp server
				*************************************************************/
                if(strncmp(rbuf, "get", 3) == 0){
                	if(mode == 0){
                         sprintf(wbuf, "%s", "PASV\r\n");
                         nwrite=6;
                         if( ( write(sockfd, wbuf, nwrite)) != nwrite){
                               printf("write error\n");
                    }
                	}
                	else{
               	        tran_sock0 = portopen(sockfd);
                        write(sockfd, wbuf, strlen(wbuf));
	                }
                    sscanf(rbuf,"get %s",re_addr);
                get=1;
                continue;
            }

				/*************************************************************
				// 8. code here: put -  put file upto ftp server
				*************************************************************/
                if(strncmp(rbuf, "put", 3) == 0){
                    if(mode == 0){
                    sscanf(rbuf,"put %s", lo_addr);
                    sprintf(wbuf, "%s", "PASV \r\n");
                    nwrite=7;
                    if( ( write(sockfd, wbuf, nwrite) ) != nwrite){
                        printf("write error\n");
                    }
                    }
                    else{
                    	sscanf(rbuf,"put %s", lo_addr);
                    	tran_sock0 = portopen(sockfd);
                        write(sockfd, wbuf, strlen(wbuf));
                    }
                    put=1;
                    continue;
                }
				/*************************************************************
				//  passive -  switch passive and active mode
				*************************************************************/
                if(strncmp(rbuf,"passive",7) == 0){
                	if(mode==0){
	                	mode = 1;
	                	printf("Entering active mode\n");
	                }
	                else{
                		mode=0;
                		printf("Entering passive mode\n");
                	}
                } 

            

			}
		}

		/* data to read from socket */
		if(FD_ISSET(sockfd, &rset)) {
			if ( (nread = recv(sockfd, rbuf, MAXBUF, 0)) < 0){
				printf("recv error\n");
            }
			//else if (nread == 0)
				//break;

			/* set replycode and wait for user's input */
			if (strncmp(rbuf, "220", 3)==0 || strncmp(rbuf, "530", 3)==0){
				strcat(rbuf,  "Your name: ");
				nread += 11;
				replycode = USERNAME;
			}

			/*************************************************************
			// 9. code here: handle other response coming from server
			*************************************************************/

			/* open data connection*/
            if(strncmp(rbuf, "331 ", 3) == 0)
            {   //printf("recv 331\n");
                strcat(rbuf, "Enter password: ");
                nread += 16;
                replycode = PASSWORD;
            }
			
            if (strncmp(rbuf, "230 ", 3) == 0){
                replycode = LOGIN;
            }
            if (strncmp(rbuf, "257", 3) == 0){
                replycode = PATHNAME;
            }
            if (strncmp(rbuf, "226", 3) == 0)
            {     //file send ok
                replycode = CLOSEDATA;
            }
            if (strncmp(rbuf, "250", 3) == 0){
                replycode = ACTIONOK;
            }
            if(strncmp(rbuf,"550",3) == 0)
            {
                replycode = 550;
            }
			/* start data transfer */
			if(write(STDOUT_FILENO, rbuf, nread) != nread){
				printf("write error to stdout\n");
            }
            if (strncmp(rbuf,"227",3)==0) {
                int port2=strtosrv(rbuf);
                
                tran_sock=cliopen(host,port2);
                if (ls==1) {
                    write(sockfd,"LIST\n",strlen("LIST\n"));
                    ftp_list(tran_sock);
                    ls=0;
                }
                else if(get==1){
                    sprintf(wbuf,"RETR %s\n",re_addr);
                    write(sockfd,wbuf,strlen(wbuf));
                    ftp_get(tran_sock,re_addr);
                    get=0;
                    
                }
                else if(put==1){
                    sprintf(wbuf,"STOR %s\n",lo_addr);
                    write(sockfd,wbuf,strlen(wbuf));
                    ftp_put(tran_sock,lo_addr);
                    put=0;
                
                }
                
                
            }
            /* start active data transfer */
            if (strncmp(rbuf,"200",3)==0){
                if (ls==1) {
                    write(sockfd,"NLST\n",strlen("NLST\n"));
                    tran_sock=portacc(tran_sock0);
                    ftp_list(tran_sock);
                    ls=0;
                }
                else if(get==1){
                    sprintf(wbuf,"RETR %s\n",re_addr);
                    write(sockfd,wbuf,strlen(wbuf));
                    tran_sock=portacc(tran_sock0);
                    ftp_get(tran_sock,re_addr);
                    get=0;
                }
                else if(put==1){
                    sprintf(wbuf,"STOR %s\n",lo_addr);
                    write(sockfd,wbuf,strlen(wbuf));
                    tran_sock=portacc(tran_sock0);
                    ftp_put(tran_sock,lo_addr);
                    put=0;
                
                }else{
                    replycode=TYPEINFORMATION;
                }
            	
            }
            if (strncmp(rbuf,"221",3)==0){
                  exit(1);
            }
            
            //if(write(STDOUT_FILENO,rbuf,nread) != nread)
                //printf("write error to stdout\n");
	}
}
	
    if(close(sockfd) < 0)
		printf("close error\n");
}


/* Read and write as data transfer connection */
void
ftp_list(int sck)
{
	int		nread;

	for ( ; ; )
	{
		/* data to read from socket */
		if ( (nread = recv(sck, rbuf, MAXBUF, 0)) < 0)
			printf("recv error\n");
		else if (nread == 0)
			break;

		if (write(STDOUT_FILENO, rbuf, nread) != nread)
			printf("send error to stdout\n");
	}

	if (close(sck) < 0)
		printf("close error\n");
}

/* download file from ftp server */
int	
ftp_get(int sck, char *pDownloadFileName_s)
{
	/*************************************************************
	// 10. code here
	*************************************************************/
    int handle=open(pDownloadFileName_s,O_WRONLY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
    int nread;
    if(dataType==0){
	for ( ; ; )
	{
		/* data to read from socket */
		if ( (nread = recv(sck, rbuf, MAXBUF, 0)) < 0)
			printf("recv error\n");
		else if (nread == 0){
			 close(handle);
					break;	
		}

        if (write(handle,rbuf,nread)!=nread) {
            printf("send error to local file\n");
        }
        
		if (write(STDOUT_FILENO, rbuf, nread) != nread)
			printf("send error to stdout\n");
	}
    }
    else if(dataType==1){
    	char *array0, *array1;
    	int byte;
    	array0 = (char *)malloc(sizeof(char));
    	//array1 = (char *)malloc(sizeof(char));
    	for( ; ; )
    	{
	    	byte = recv(sck, array0,1, 0);
	    	if(!byte){
	    		printf("Download file successfully\n");
	    		close(handle);
	    	    break;
	    	}
	    	if(array0[0]=='\r'){
	    	}
	    	else if(write(handle, array0, byte)!=byte)
	    	printf("write error\n");
	    	}
    	
    }
	if (close(sck) < 0)
		printf("close error\n");
    
    return 0;
    
}

/* upload file to ftp server */
int 
ftp_put (int sck, char *pUploadFileName_s)
{
	/*************************************************************
	// 11. code here
	*************************************************************/
    int handle=open(pUploadFileName_s,O_RDWR);
    int nread;
    if (handle==-1) {
        printf("Faile to open the File");
    }
    if(dataType==0){
    for ( ; ; )
	{
		/* data to read from socket */
		if ( (nread = read(handle, rbuf, MAXBUF)) < 0)
			printf("read error\n");
		else if (nread == 0){
			close(handle);
			break;			
		}

        if (write(STDOUT_FILENO,rbuf,nread)!=nread) {
            printf("send error from local file\n");
        }
        
		if (write(sck, rbuf, nread) != nread)
			printf("send error to socket\n");
	}
    }
    else if(dataType==1){
    	char *array;
    	int byte;
    	FILE *stream;
    	if((stream=fopen(pUploadFileName_s,"rb"))==NULL){
	    	printf("fopen error\n");
	    	exit(1);
	    }
    	array = (char *)malloc(sizeof(char)*2);
    	for( ; ; )
    	{
	    	byte = fread(array, sizeof(char),1, stream);
	    	if(!byte){
	    		printf("Upload file successfully");
	    		fclose(stream);
	    	    break;
	    	}
	    	if(array[0]=='\n'){
	    		array[0]='\r';
	    		array[1]='\n';
	    		if(write(sck, array, 2)!=2)
	    		printf("send seeror\n");
	    	}
	    	else if(write(sck, array, byte)!=byte)
	    	printf("send error\n");
	    	}
	    	}
    
	if (close(sck) < 0)
		printf("close error\n");
    
    return 0;
}
