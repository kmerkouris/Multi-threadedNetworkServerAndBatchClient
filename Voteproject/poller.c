#include <sys/types.h>                                  
#include <sys/socket.h>                                 
#include <netinet/in.h>                        
#include <netdb.h>                              
#include <stdio.h>                                         
#include <stdlib.h>                                     
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>



struct arguments
{
    int bufferSize;
    int *buffer;
    int bufferCount;
    int inCounter;
    int outCounter;
};

struct votes
{
    int socketdesc;
    char *nameVoter;
    char *partyVoted;
    char *poll_log;
    char *poll_stats;
    int voted;
};

struct statisticsPARTY
{
    char *party;
    int counter;
};

struct create
{
    struct arguments *a;
    struct votes *v;
};

//init mutexes and cond variables
pthread_mutex_t mutexBuffer = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fullBufferCond =  PTHREAD_COND_INITIALIZER;
pthread_cond_t emptyBufferCond =  PTHREAD_COND_INITIALIZER;


void addToBuffer(int newsock, void* argument)
{
    struct arguments* argus = (struct arguments*) argument;

    sleep(1);

    pthread_mutex_lock(&mutexBuffer);
    //oso eiani gematos o buffer wait thn fullcond
    while (argus->bufferCount == argus->bufferSize) 
    {
        pthread_cond_wait(&fullBufferCond, &mutexBuffer);
    }

    argus->buffer[argus->inCounter] = newsock;

    //diaxeirisi pointer twn eisodwn sto buffer
    argus->inCounter++;
    if(argus->inCounter == argus->bufferSize)
    {
        argus->inCounter = 0;
    }

    argus->bufferCount++;

    pthread_cond_signal(&emptyBufferCond);
    pthread_mutex_unlock(&mutexBuffer);
}

int removeFromBuffer(void* argument)
{
    struct arguments* argus = (struct arguments*) argument;


    pthread_mutex_lock(&mutexBuffer);
    //oso einai adeios o buffer wait thn empty cond
    while (argus->bufferCount == 0) 
    {
        pthread_cond_wait(&emptyBufferCond, &mutexBuffer);
    }

    int socketfd = argus->buffer[argus->outCounter];
    //diaxeirish pointer twn eksodwn toy buffer
    argus->outCounter++;
    if(argus->outCounter == argus->bufferSize)
    {
        argus->outCounter = 0;
    }

    argus->bufferCount--;

    pthread_cond_signal(&fullBufferCond);
    pthread_mutex_unlock(&mutexBuffer);

    return socketfd;
}

void *createThreads(void* create)
{
    struct create* cr = (struct create*) create;
    struct votes* Vote = (struct votes*) cr->v;
    struct arguments* argus = (struct arguments*) cr->a;
    
    sleep(1);

    while (1)
    {
        int socketdesc = removeFromBuffer(argus); // epistrefei ton socket descriptor pou exei antlisei apo to buffer
        //diavazei ta mhnumata stou client afoy exei steilei aithmata 
        char *str = "SEND NAME PLEASE";
        if (write(socketdesc, str, strlen(str)+1) < 0)
        {
            perror("write");
            exit(1);
        }

        char buffer[1024];

        for(int i = 0; i < 2; i++)
        {
            memset(buffer, 0, sizeof(buffer));
            if (read(socketdesc, buffer, sizeof(buffer)) < 0) {
                perror("read");
                close(socketdesc);
                exit(1);
            }

            if(i==0)
                memcpy(Vote->nameVoter,buffer,sizeof(buffer)+1);
            else
            {
                strcat(Vote->nameVoter, " ");
                strcat(Vote->nameVoter, buffer);
            }

        }

        printf("%s\n", Vote->nameVoter);

        if(Vote->voted != 0)
        {
            sleep(1);
            char *msg = "ALREADY VOTED";
            if (write(socketdesc, msg, strlen(msg)+10) < 0)
            {
                perror("write");
                exit(1);
            }
        }
        else
        {

            char *msg = "SEND VOTE PLEASE";

            if(write(socketdesc, msg, sizeof(msg)+10) < 0)
            {
                perror("write");
                exit(1);
            }

            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));
            if (read(socketdesc, buffer, sizeof(buffer)) < 0) {
                perror("read");
                close(socketdesc);
                exit(1);
            }

            memcpy(Vote->partyVoted,buffer,sizeof(buffer)+1);
            printf("%s\n", Vote->partyVoted);

            //record the vote
            FILE *poll_log = fopen(Vote->poll_log, "a");
            fprintf(poll_log, "%s %s\n", Vote->nameVoter, Vote->partyVoted);
            fclose(poll_log);

            // Update the statistics file
            FILE *poll_stats = fopen(Vote->poll_stats, "a");
            fprintf(poll_stats, "%s : %d\n", Vote->partyVoted, 1);
            fclose(poll_stats);

            // sleep(1);
            char vote_msg[100];
            sprintf(vote_msg, "VOTE for Party RECORDED %s", Vote->partyVoted);
            if (write(socketdesc, vote_msg, sizeof(vote_msg)) < 0 )
            {
                perror("write");
                exit(1);
            }
            
        }
        printf("Closing connection.\n");
        sleep(1);
        close(socketdesc);

    }
    
}


int main(int argc , char *argv[])
{
    //to master thread einai to default thread ths main 

    int sock, newsock, bufferSize, portnum, numWorkerthreads;

    struct sockaddr_in server, client;
    struct sockaddr *serverptr=(struct sockaddr *)&server;
    struct sockaddr *clientptr=(struct sockaddr *)&client;
    socklen_t clientlen = sizeof(client);
 
    
    if (argc != 6)
    {
        printf("usage %s , Give portnum , numWorkerthreads ,  bufferSize , poll_log and pol_stats\n",argv[0]);
        exit(1);
    }

    portnum = atoi( argv[1] );

    if ( (numWorkerthreads = atoi(argv[2])) <= 0)
    {
        perror("num of thread wrong input");
        exit(1);
    }
    
    if ( (bufferSize = atoi(argv[3])) <= 0)
    {
        perror("buffer size wrong input");
        exit(1);
    }
    
    //socket create
    if ((sock = socket(AF_INET, SOCK_STREAM , 0)) < 0)
    {
        perror("socket");
        exit(1);
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);//des kai to memcpy apo to client
    server.sin_port = htons(portnum);

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)     //reuse port or address that is already running operations, solution to "Address already in use"
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }   

    if (bind(sock, serverptr, sizeof(server)) < 0)
    {
        perror("bind");
        exit(1);
    }

    if (listen(sock,500) < 0)
    {
        perror("listen");
        exit(1);
    }
    printf("Listening for connections to port %d\n", portnum);

    //buffer create 
    int buffer[bufferSize];

    struct arguments *ar;
    ar= (struct arguments*)malloc(sizeof(struct arguments));
    ar->bufferSize = bufferSize;
    ar->buffer = (int*)malloc(sizeof(int)*bufferSize);
    ar->buffer = buffer;
    ar->bufferCount = 0;
    ar->inCounter = 0;
    ar->outCounter = 0;

    //votes info 
    struct votes *vote;
    vote=(struct votes*)malloc(sizeof(struct votes));
    vote->nameVoter=(char*)malloc(bufferSize);//i ena allo noumero anti gia buffersize
    vote->partyVoted=(char*)malloc(bufferSize);
    vote->poll_log = argv[4];
    vote->poll_stats = argv[5];
    vote->voted = 0;

    struct create *create;
    create=(struct create*)malloc(sizeof(struct create));
    create->a = ar;
    create->v = vote;

    pthread_t workerThreads[numWorkerthreads];

    //create threads to serve connection to client
    for (int i = 0; i < numWorkerthreads; i++)
    {
        if (pthread_create(&workerThreads[i], NULL, createThreads, (void *)create))
        {
            perror("pthread_create");
            exit(1);
        }
    }

    while(1)
    {
        if ((newsock = accept(sock, clientptr, &clientlen)) < 0)
        {
            perror("accept");
            exit(1);
        }
        printf("Accepted connection!");
       
        addToBuffer(newsock, (void*) ar);
    }

    sleep(1);
    
    for (int i = 0; i < numWorkerthreads; ++i) {
        if (pthread_join(workerThreads[i], NULL) )
        {
            printf("join ss\n");
            exit(1);
        }
    }

    return 0;
}