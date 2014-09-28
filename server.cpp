/*
 ** server.c -- a stream socket server demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <pthread.h>
#include <math.h>

using namespace std;


#define BACKLOG 10     // how many pending connections queue will hold
#define MAXDATASIZE 100
//*********************global var
#define max_connect 10
#define KEY 78278
char * PORT;
char buf[MAXDATASIZE];
pthread_mutex_t m_connections,m_XO;
int XO=0;
int connections=0;
int sockfd, new_fd[max_connect];
int fin_listen=0;
int x_sock_id,o_sock_id;
int busy_sock[max_connect];

struct addrinfo hints, *servinfo, *p;
struct sockaddr_storage their_addr; // connector's address information
socklen_t sin_size;
struct sigaction sa;
int yes=1;
char s[INET6_ADDRSTRLEN];
int rv;


//*********************
void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void * test_client(void * in){
	int * j=(int *)in;
	int num=*j;
	int numbytes;
	int turn=0;
	uint32_t in_key;

	if ((numbytes = recv(new_fd[num],&in_key, 4, 0)) == -1) {
		perror("recv");
		exit(1);
	}

	if(in_key!=KEY){
		close(new_fd[num]);
		pthread_mutex_lock(&m_connections);
		busy_sock[num]=-1;
		pthread_mutex_unlock(&m_connections);

		cerr<<"connection is aborted due to wrong key code!"<<endl;
	}

	else{

		pthread_mutex_lock(&m_XO);

		if(XO<1){

			if(XO==0){
				x_sock_id=num;
				cout<<"Player X is connected."<<endl;
			}
			XO++;
		}
		else if(XO==1)
		{
			o_sock_id=num;
			cout<<"Player O is connected."<<endl;
			fin_listen=1;
			XO++;
		}
		else
			fin_listen=1;

		pthread_mutex_unlock(&m_XO);

	}
}

void * listener(void * in){

	pthread_t th_connect[max_connect];


	for(int i=0;i<max_connect;i++)
		busy_sock[i]=-1;

	int j=0;
	while(1) {  // main accept() loop
		if(fin_listen){
				//	close(new_fd[j]);
					return 0;
				}

		sin_size = sizeof their_addr;
		int i;
		pthread_mutex_lock(&m_connections);

		for(i=0;i<max_connect;i++)
			if(busy_sock[i]==-1){
				j=i;
				break;
			}
		pthread_mutex_unlock(&m_connections);
		if(i==max_connect)
			continue;

		if(!fin_listen)
			new_fd[j] = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		else
			break;

		if (new_fd[j] == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);

		if(fin_listen){
					close(new_fd[j]);
					return 0;
				}

		printf("server: got connection from %s\n", s);


		pthread_mutex_lock(&m_connections);
		connections++;
		pthread_mutex_unlock(&m_connections);


		busy_sock[j]=j;
		pthread_create( &th_connect[j], NULL, test_client , &busy_sock[j]);


	}
}

void make_state(uint32_t *state, int input,bool prev_turn){
	uint32_t six=6,nine=9,one=1; //x->9
	uint32_t six_mask=0xffffffff,nine_mask=0xffffffff;
	six_mask=six_mask ^ (nine<<25);

	nine_mask=nine_mask ^ (six<<25);

	if(prev_turn){
		*state=*state & nine_mask;

		*state=*state | (nine<<25);
		*state=*state | one<<input;
	}
	else {
		*state=*state & six_mask;
		*state=*state | (six<<25);
		*state=*state | one<<(input+16);
	}


}

bool check_end(int *table){
	uint32_t state;
	int nine=9,six=6;
	int one=1;
	int x_cnt=0,o_cnt=0;
	int win_state=0; //1->x wins. 2-> o wins
	for(int i=0;i<3;i++){
		for(int j=0;j<3;j++){
			if(table[i*3+j]==1)
				x_cnt++;
			else if(table[i*3+j]==2)
				o_cnt++;
		}
		if(x_cnt==3){
			win_state=1;
			break;
		}
		else if(o_cnt==3){
			win_state=2;
			break;
		}
		x_cnt=0;
		o_cnt=0;
	}

	if(win_state==0){
		x_cnt=0;
		o_cnt=0;
		for(int j=0;j<3;j++){
				for(int i=0;i<3;i++){
					if(table[i*3+j]==1)
						x_cnt++;
					else if(table[i*3+j]==2)
						o_cnt++;
				}
				if(x_cnt==3){
					win_state=1;
					break;
				}
				else if(o_cnt==3){
					win_state=2;
					break;
				}
				x_cnt=0;
				o_cnt=0;
			}
	}
	if((table[0]==1 && table[4]==1 && table[8]==1) || (table[2]==1 && table[4]==1 && table[6]==1))
		win_state=1;
	else if((table[0]==2 && table[4]==2 && table[8]==2) || (table[2]==2 && table[4]==2 && table[6]==2))
		win_state=2;

	if(win_state ==1){
		state=nine<<25 | one<<31;
		cout<<"The X won the game!"<<endl;

	}
	else if(win_state==2){
		state=six<<25 | one<<31;
		cout<<"The O won the game!"<<endl;
	}
	if(win_state!=0){

		if (send(new_fd[x_sock_id],&state, 4, 0) == -1)
			perror("send");

		if (send(new_fd[o_sock_id],&state, 4, 0) == -1)
			perror("send");
	}
	else{ //check for tie
		for(int k=0;k<9;k++)
			if(table[k]==0)
				return 0;

		state=one<<31;
		if (send(new_fd[x_sock_id],&state, 4, 0) == -1)
			perror("send");

		if (send(new_fd[o_sock_id],&state, 4, 0) == -1)
		perror("send");

		cout<<"The game is drawn!"<<endl;
		return 1;

	}


}

void game(){
	int table[9]={0,0,0,0,0,0,0,0,0}; //1->x 2->y
	bool turn=0;//turn=0-> x   turn=1->y
	int numbytes;
	uint32_t state_sent,state_recv;
	int six=6,nine=9; //x->9
	state_sent=nine<<25;
	bool end=false;

	while(1){


		if(!turn){
			if (send(new_fd[x_sock_id],&state_sent, 4, 0) == -1)
				perror("send");
		}
		else{
			if (send(new_fd[o_sock_id],&state_sent, 4, 0) == -1)
				perror("send");
		}

		if(!turn){
			if ((numbytes = recv(new_fd[x_sock_id],&state_recv, 4, 0)) == -1) {
				perror("recv");
				exit(1);
			}
		}
		else{
			if ((numbytes = recv(new_fd[o_sock_id],&state_recv, 4, 0)) == -1) {
				perror("recv");
				exit(1);
			}
		}

		int input=log(state_recv)/log(2);


		if(table[input]==0)
		{
			if(!turn)
				table[input]=1;
			else table[input]=2;
			make_state(&state_sent, input,turn);
			turn=!turn;
			if(check_end(table))
				return;
		}
		//else cout<<"wrong input1!"<<endl;
	}
}
int main(int argc, char* argv[])
{
	pthread_mutex_init(&m_connections,NULL);
	pthread_mutex_init(&m_XO,NULL);

	pthread_t th_listener;


	if(argc!=2)
	{
		cerr<<"wrong input!"<<endl;
		exit(1);
	}

	PORT =argv[1];



	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	pthread_create( &th_listener, NULL, listener , NULL);

	while(!fin_listen){};
	//finished waiting for two players

	game();


	close(new_fd[x_sock_id]);
	close(new_fd[o_sock_id]);
	close(sockfd);
	return 0;
}
