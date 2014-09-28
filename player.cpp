/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
using namespace std;

#define MAXDATASIZE 100 // max number of bytes we can get at once
#define KEY 78278

//************* global var
uint32_t key=KEY;



char * PORT;
//******************

//0-8 -> O=2   16-24 -> X=1
void print(uint32_t state){
	int table[9]={0,0,0,0,0,0,0,0,0};
	int one=1,temp;
	for(int i=0;i<9;i++){
		temp=one;
		if (temp & state)
			table[i]=2;

		one=one<<1;
	}

	one=1;
	one=one<<16;
	for(int i=0;i<9;i++){
				if (one & state)
					table[i]=1;

				one=one<<1;
	}

	printf("\033[2J\033[1;1H");

	for(int i=0;i<3;i++){
		for(int j=0;j<3;j++){
		if(table[i*3+j]==0)
			cout<<" ";
		else if(table[i*3+j]==1)
			cout<<"X";
		else if(table[i*3+j]==2)
			cout<<"O";
		if(j!=2) cout<<"|";
		}
		cout<<endl;
	}
	one=1;
	one=one<<25;
	if(one & state)
	cout<<"Move as X: ";
	else
		cout<<"Move as O: ";

}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{

	uint32_t state_server;
	uint32_t state_client;


	//*key=KEY;
    int sockfd, new_fd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 3) {
       cerr<<"wrong input!"<<endl;
        exit(1);
    }
	PORT=argv[2];
	//cout<<*PORT<<endl;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure
/*
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';
    printf("client: received '%s'\n",buf);
    */
	//char * temp=&(char *)key;

	if (send(sockfd,&key, 4, 0) == -1)
           perror("send");
	//key is sent

    int input;
    int one=1,nine=9,six=6;
    while(1){
    	if ((numbytes = recv(sockfd, &state_server,4, 0)) == -1) {
    	        perror("recv");
    	        exit(1);
    	    }
          if(state_server & one<<31){
        	  if(state_server & nine<<25){
        		  printf("\033[2J\033[1;1H");
        		  cout<<"The X won the game!"<<endl;

        	  }

        	  else if(state_server & six<<25){
        		  printf("\033[2J\033[1;1H");
        		  cout<<"The O won the game!"<<endl;

        	  }
        	  else{
        		  printf("\033[2J\033[1;1H");
        		  cout<<"The game is drawn!"<<endl;
        	  }
        	  break;
          }
    	  print(state_server);

    	  cin>>input;
    	  while(input<1 || input>9){

    		  cout<<"Input must be form 1 to 9. Please enter another number: "<<endl;
    		  cin>>input;
    	  }
    	  input--;

    	  state_client=one<<input;

    	 // cout<<"state_client "<<state_client<<endl;

    	  if (send(sockfd,&state_client, 4, 0) == -1)
    	             perror("send");


    }







    close(sockfd);

    return 0;
}
