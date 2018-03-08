#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<signal.h>
#include<netinet/in.h>
#include<sys/select.h>
#include<fcntl.h>
void handle_write(struct sockaddr_in *remote, char buffer[], unsigned len);
void handle_read(struct sockaddr_in *remote, char buffer[], unsigned len);


/*Sigfunc * signal (int signo, Sigfunc *func)
{
	struct sigaction act, oact;
	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags =0;
	if(signo == SIGALRM){
#ifdef SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;
#endif
} else {
#ifdef SA_RESTART 
	act.sa_flags |= SA_RESTART;
#endif 
}

	if(sigaction(signo,&act,&oact)<0)
	return (SIG_ERR);
return (oact.sa_handler);

}
*/

#define BUF_LEN 516
int main() {
    ssize_t n;
    char buffer[BUF_LEN];
    socklen_t sockaddr_len;
    int server_socket;
    struct sigaction act;
    unsigned short int opcode;
    unsigned short int * opcode_ptr;
    struct sockaddr_in sock_info;
   
    /* Set up interrupt handlers */
  /*  act.sa_handler = sig_chld;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, NULL);
    
    act.sa_handler = sig_alarm;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGALRM, &act, NULL);
*/   
    sockaddr_len = sizeof(sock_info);
    
    /* Set up UDP socket */
    memset(&sock_info, 0, sockaddr_len);
    
    sock_info.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_info.sin_port = htons(INADDR_ANY);
    sock_info.sin_family = PF_INET;
    
    if((server_socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(-1);
    }
    
    if(bind(server_socket, (struct sockaddr *)&sock_info, sockaddr_len) < 0) {
        perror("bind");
        exit(-1);
    }
    
    /* Get port and print it */
    getsockname(server_socket, (struct sockaddr *)&sock_info, &sockaddr_len);
    
    printf("%d\n", ntohs(sock_info.sin_port));
    
    /* Receive the first packet and deal w/ it accordingly */
    while(1) {
    intr_recv:
        n = recvfrom(server_socket, buffer, BUF_LEN, 0,
                     (struct sockaddr *)&sock_info, &sockaddr_len);
        if(n < 0) {
            if(errno == EINTR) goto intr_recv;
            perror("recvfrom");
            exit(-1);
        }
        /* check the opcode */
        opcode_ptr = (unsigned short int *)buffer;
        opcode = ntohs(*opcode_ptr);
        if(opcode != 1 && opcode != 2) {
            /* Illegal TFTP Operation */
            *opcode_ptr = htons(5);
            *(opcode_ptr + 1) = htons(4);
            *(buffer + 4) = 0;
        intr_send:
            n = sendto(server_socket, buffer, 5, 0,
                       (struct sockaddr *)&sock_info, sockaddr_len);
            if(n < 0) {
                if(errno == EINTR) goto intr_send;
                perror("sendto");
                exit(-1);
            }
        }
        else {
            if(fork() == 0) {
                /* Child - handle the request */
                close(server_socket);
                break;
            }
            else {
                /* Parent - continue to wait */
            }
        }
    }
    
    if(opcode == 01) handle_read(&sock_info, buffer, BUF_LEN);
    if(opcode == 02) handle_write(&sock_info, buffer, BUF_LEN);
    
    return 0;
}

//static void sig_alarm(int);

void handle_read(struct sockaddr_in *remote, char buffer[], unsigned len)
{

	ssize_t n;
	int fd;
	int sock;
	uint16_t block;
	int attempts;
	ssize_t last_size;
	socklen_t sockaddr_len;
	fd_set rfds;
	struct timeval tv;
	int retval;
	
	struct sockaddr_in temp;
	unsigned short int opcode;
	unsigned short int * opcode_ptr;
	char fileData[BUF_LEN];

	printf("RRQ\n");
	opcode_ptr = (unsigned short int *)buffer;

	sockaddr_len = sizeof(temp);
	memset(&temp, 0, sockaddr_len);
	temp.sin_addr.s_addr = htonl(INADDR_ANY);
	temp.sin_port = htons(INADDR_ANY);
	temp.sin_family = AF_INET;

	
	if((sock = socket(AF_INET, SOCK_DGRAM, 0))<0)	{
		perror("socket");
		exit(-1);
	}
	if(bind(sock, (struct sockaddr *)&temp, sockaddr_len)<0)	{
		perror("bind");
		exit(-1);
	}

	if((fd = open((buffer + 2), O_RDONLY))<0)	{
	/*file not found */
	printf("file not found\n");
	*opcode_ptr = htons(5);
	*(opcode_ptr + 1) = htons(1); //file not found error
	*(buffer + 4) = 0;

	fnf_send:
		n = sendto(sock,buffer,5,0,(struct sockaddr *)remote, sockaddr_len);
		if(n<0)	{
		if(errno == EINTR) goto fnf_send;
		perror("sendto");
		exit(-1);
		}
			
	}

	//file exists, continue reading and sending 
	//Signal(SIGALRM, sig_alrm);
	int abort=0;
	int byte_read;
	int attempt=0;
	int count;
 	block = 1;
	*opcode_ptr = htons(3);	

	*(opcode_ptr + 1) = htons(block);	
	while((byte_read = read(fd, (void *)(buffer+4), 512))>0)
	{
		//printf("File block read %d\n", block);
		
		packetsend:
		//printf("Sending Packed number %d\n", block);
	//	printf("%u%u%s\n",*(opcode_ptr), *(opcode_ptr+1), buffer+4);
		++attempt;
		n = sendto(sock, buffer, byte_read+4, 0, (struct sockaddr *)remote, sockaddr_len);
			if(n<0)	{
			if(errno == EINTR) goto packetsend;
			perror("Packet unable to send");
			exit(-1);
			}
			
	ack_recv:
//	printf("Receiving acknowledgement \n");
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	retval = select(1, &rfds, NULL, NULL, &tv); //waits for 1 sec
//	if(retval)
	{
        n = recvfrom(sock, buffer, BUF_LEN, 0, (struct sockaddr *)remote, &sockaddr_len);
	++abort;
	if(abort==10)
		{
		perror("Waited for 10 seconds..connection aborted\n");
		exit(-1);
		}
	}
	if(n < 0) {
            if(errno == EINTR) goto packetsend;
            perror("recvfrom");
            exit(-1);
		}	
	printf("%u  %u\n",*(buffer+1),*(buffer+3));
	count = ntohs(*(opcode_ptr+1));
	if(count != block)
		{
		if(attempt==5)
		{
		perror("Error Sending packet..premature termination\n");
		break;
		}
		else
		goto packetsend;
		}
	attempt=0;
        abort = 0;
        opcode = ntohs(*opcode_ptr);
	//printf("Opcode is %u\n",opcode);
	//printf("Block number is %u\n",*(opcode_ptr+1));        
	if(opcode != 04) {
            /* Illegal TFTP Operation */
            *opcode_ptr = htons(05);
            *(opcode_ptr + 1) = htons(4);
            *(buffer + 4) = 0;
        opcode_send:
            n = sendto(sock, buffer, 5, 0,
                       (struct sockaddr *)remote, sockaddr_len);
            if(n < 0) {
                if(errno == EINTR) goto opcode_send;
                perror("opcode sendto");
                exit(-1);
		}
		}
	++block;
		
		*opcode_ptr = htons(3);	
		*(opcode_ptr + 1) = htons(block);		

	}
	printf("Read terminated correctly\n");

}
void handle_write(struct sockaddr_in *remote, char buffer[], unsigned len)
{
    printf(" i will write locally \n");
        ssize_t n;
	int fd; int fw;
	int sock;
	int block;
	
	ssize_t last_size;
	socklen_t sockaddr_len;
	char r_buffer[BUF_LEN];
        struct timeval tv;
        int retval;
	struct sockaddr_in temp;
	unsigned short int opcode;
	unsigned short int * opcode_ptr , * data_ptr;  
        int rec_buff_len=0;
        fd_set rfds; 
        opcode_ptr = (unsigned short int *)buffer;

	sockaddr_len = sizeof(temp);
	memset(&temp, 0, sockaddr_len);
	temp.sin_addr.s_addr = htonl(INADDR_ANY);
	temp.sin_port = htons(INADDR_ANY);
	temp.sin_family = PF_INET;


	if((sock = socket(PF_INET, SOCK_DGRAM, 0))<0)	{
		perror("socket");
		exit(-1);
	}
	if(bind(sock, (struct sockaddr *)&temp, sockaddr_len)<0)	{
		perror("bind");
		exit(-1);
	}
        // created socket and binded
        int data_number = 0;
        int flag = 1;
        //Signal(SIGALRM, sig_alarm);
        int attempts=0;
        int abort = 0;
        
        printf("filename : %s \n",buffer+2);
       
       ///// fw = open((buffer + 2), O_WRONLY|O_CREAT,00777);
       //if(fw == -1 )
       //{    
       //     FILE* file_ptr = fopen((buffer + 2), "w");
       //     fclose(file_ptr);
       //}
       //close(fw);
       // fw = open((buffer + 2), O_WRONLY);        
     
        if((fd = open((buffer + 2), O_WRONLY))>0)	
        {
	  /*file not found */
	  printf("file already exist \n");
	  *opcode_ptr = htons(5);
	  *(opcode_ptr + 1) = htons(6); //file already exist error
	  *(buffer + 4) = 0;
	  fnf_s:
	      n = sendto(sock,buffer,5,0,(struct sockaddr *)remote, sockaddr_len);
	      if(n<0)	{
	      if(errno == EINTR) goto fnf_s;
	      perror("sendto");
	      exit(-1);
	      }	
	}

        fw = open((buffer + 2), O_WRONLY|O_CREAT,00777);       
        while( flag ==1)
        {
           *opcode_ptr = htons(04);
	   *(opcode_ptr + 1) = htons(data_number);

	   fnf_send:
              ++attempts;
	      n = sendto(sock,buffer,4,0,(struct sockaddr *)remote, sockaddr_len);
	      if(n<0)	
              {
	        if(errno == EINTR) goto fnf_send;
	         perror("sendto");
	         exit(-1);
	      }
           
           intr_recv:
           FD_ZERO(&rfds);
	   FD_SET(0, &rfds);
	   tv.tv_sec = 1;
	   tv.tv_usec = 0;
	   retval = select(1, &rfds, NULL, NULL, &tv);
           n = recvfrom(sock, r_buffer, BUF_LEN, 0, (struct sockaddr *)&temp, &sockaddr_len);
           ++abort;
	   if(abort==10)
		{
		perror("Waited for 10 seconds..connection aborted\n");
		exit(-1);
		}
           if(n < 0) 
           {
             if(errno == EINTR) goto fnf_send;
             perror("recvfrom");
             exit(-1);
           }
           data_ptr = (unsigned short int *)(r_buffer+2);
           printf("data buffer : %u \n", ntohs(*data_ptr));
           //printf("data buffer : %u \n", data_ptr);
           if( ntohs(*data_ptr)!= data_number+1)
		{
		if(attempts==5)
		  {
		     perror("Error Sending packet..premature termination\n");
		     break;
	   	  }
		else
		goto fnf_send;
		}
          
           attempts = 0;
           data_number +=1 ;
           write(fw, r_buffer+4 , n-4); 
           //printf("%s", r_buffer+4); 
        
           if( n < 516)
           {  
              flag = 0; printf("flag is 0 ");
              *opcode_ptr = htons(04);
	      *(opcode_ptr + 1) = htons(data_number);
	      fnf_send2:
		 n = sendto(sock,buffer,4,0,(struct sockaddr *)remote, sockaddr_len);
		 if(n<0)	
                 {
		   if(errno == EINTR) goto fnf_send2;
		   perror("sendto");
		   exit(-1);
		 } 
           }
         }
         printf("done receiving");
         close(fw);
         return;
}



