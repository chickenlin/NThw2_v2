#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 1234
#define MAXDATASIZE 100

char sendbuf[1024];
char recvbuf[1024];
char name[100];
int fd;
int board[9];

void usage(){
    //printf("Change user name, input: 1 YourName\n");
    printf("Show all user, input: 2\n");
    printf("To invite other player, input: 3 YourName Other'sName\n");
    printf("To quit, input: quit\n\n");
}

void print_board(int *board){
    printf("\n");
    printf("%d  %d  %d \n", board[0], board[1], board[2]);
    printf("\n");
    printf("%d  %d  %d \n", board[3], board[4], board[5]);
    printf("\n");
    printf("%d  %d  %d \n\n\n", board[6], board[7], board[8]);
}

int choose_user_turn(int *board){
    int i=0;
    int inviter=0, invitee=0;
    for(i=0; i<9; i++){
        if(board[i] == 1){
            inviter++;
        }
        else if(board[i] == 2){
            invitee++;
        }
    }
    if(inviter > invitee)
        return 2;
    else
        return 1;
}

// modify chess board, and fill "sendbuf" with package format.
void write_on_board(int *board, int location){
    print_board(board);
    int user_choice = choose_user_turn(board);
    // Record which location on board is selected by inviter.
    board[location] = user_choice;
    sprintf(sendbuf, "7  %d %d %d %d %d %d %d %d %d\n", board[0], \
        board[1],board[2],board[3],board[4],board[5],board[6],board[7],board[8]);
}



// Only handle message from server to client.
void pthread_recv(void* ptr)
{
    int instruction;
    while(1)
    {
        memset(sendbuf,0,sizeof(sendbuf));
        instruction = 0;
        // recvbuf is filled by server's fd.
        if ((recv(fd,recvbuf,MAXDATASIZE,0)) == -1)
        {
            printf("recv() error\n");
            exit(1);
        }
        sscanf (recvbuf,"%d",&instruction);
        switch (instruction)
        {
            case 2: {
                printf("%s\n", &recvbuf[2]); // Print the message behind the instruction.
                break;
            }
            case 4: {
                char inviter[100];
                sscanf(recvbuf,"%d %s",&instruction, inviter);
                printf("%s\n", &recvbuf[2]); // Print the message behind the instruction.
                printf("Accept:5 1 %s\n", inviter);
                printf("Refuse:5 0 %s\n", inviter);
                break;
            }
            case 6: {
                printf("You can Start your game\n\n");
                printf("Inviter is 1\n");
                printf("Invitee is 2\n");
                printf("Please input:x_axis,y_axis\n");
                print_board(board);
                break;
            }
            case 8: {
                // int board[9];
                char msg[100];
                sscanf (recvbuf,"%d %d%d%d%d%d%d%d%d%d %s",&instruction,&board[0],&board[1],&board[2],&board[3],&board[4],&board[5],&board[6],&board[7],&board[8], msg);
                print_board(board);
                printf("%s\n", msg);
                printf("Please input:x_axis,y_axis\n");
                break;
            }
            
            default:
                break;
        }   

        memset(recvbuf,0,sizeof(recvbuf));
    }
}



int main(int argc, char *argv[])
{
    int  numbytes;
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in server;


    if (argc !=2)
    {
        printf("Usage: %s <IP Address>\n",argv[0]);
        exit(1);
    }


    if ((he=gethostbyname(argv[1]))==NULL)
    {
        printf("gethostbyname() error\n");
        exit(1);
    }

    if ((fd=socket(AF_INET, SOCK_STREAM, 0))==-1)
    {
        printf("socket() error\n");
        exit(1);
    }

    bzero(&server,sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr = *((struct in_addr *)he->h_addr);
    if(connect(fd, (struct sockaddr *)&server,sizeof(struct sockaddr))==-1)
    {
        printf("connect() error\n");
        exit(1);
    }

    // First, Add User.
    printf("Connect success\n");
    char str[]=" have come in\n";
    printf("Please ENTER your user name：");
    fgets(name,sizeof(name),stdin);
    char package[100];
    strcat(package, "1 ");
    strcat(package, name);
    send(fd, package, (strlen(package)),0);

    // usage
    usage();

    // Only handle message from server to client. (Goto pthread_recv finction)
    pthread_t tid;
    pthread_create(&tid, NULL, (void*)pthread_recv, NULL);
    // Only handle message from client to server.
    while(1){
        memset(sendbuf,0,sizeof(sendbuf));
        fgets(sendbuf,sizeof(sendbuf),stdin);   // Input instructions
        int location;
        if(sendbuf[1] == ','){
			int a, b;
			sscanf(&sendbuf[0], "%d", &a);
            sscanf(&sendbuf[2], "%d", &b);
			if(a == 1){
				if(b == 1) location = 0;
				else if(b == 2) location = 1;
				else location = 2;
			}
			else if(a == 2){
				if(b == 1) location = 3;
				else if(b == 2) location = 4;
				else location = 5;
			}
			else{
				if(b == 1) location = 6;
				else  if(b == 2) location = 7;
				else location = 8;
			}
            write_on_board(board, location);
        }
        send(fd,sendbuf,(strlen(sendbuf)),0);   // Send instructions to server
        // Logout
        if(strcmp(sendbuf,"quit\n")==0){          
            memset(sendbuf,0,sizeof(sendbuf));
            printf("Quit.\n");
            return 0;
        }
    }
    pthread_join(tid,NULL);
    close(fd);
    return 0;
}
