//batch multithreaded client //
#include <sys/types.h>                                
#include <sys/socket.h>                                 
#include <netinet/in.h>                         
#include <arpa/inet.h>
#include <netdb.h>                              
#include <stdio.h>                                         
#include <stdlib.h>                                         
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

//init mutex and con variable 
pthread_mutex_t mutexBuff = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int ready = 0;

struct arguments
{
    struct hostent *serverName;
    int portNum;
    FILE *file;
    char** voteArguments;
};

void sendVote(void *argg) 
{
    sleep(1);

    struct arguments *argus = (struct arguments*) argg;

    //socket create
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM , 0)) < 0)
    {
        perror("socket");
        exit(1);
    }

    //server
    struct sockaddr_in server;
    
    server.sin_family = AF_INET; //internet domain
    memcpy(&server.sin_addr, argus->serverName->h_addr, argus->serverName->h_length);//copy in server sin_address the hosts adress
    server.sin_port = htons(argus->portNum);//htons - converts the unsigned short integer hostshort from 1)host byte order to 2)network byte order


    struct sockaddr *serverptr = (struct sockaddr*)&server;
    if(connect(sock,serverptr,sizeof(server)) < 0)
    {
        perror("connect");
        close(sock);
        exit(1);
    }
    //lambanei aithma gia psifoys apo ton server kai antapokrinetai mesw write grafontas ta katallhla dedomena
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    if (read(sock, buffer, sizeof(buffer)) < 0) {
        perror("read");
        close(sock);
        exit(1);
    }
    printf("%s\n", buffer);

    for(int i = 0; i < 2; i++)
    {
        if (write(sock, argus->voteArguments[i], strlen(argus->voteArguments[i])+1) < 0)
        {
            perror("write");
            close(sock);
            exit(1);
        }
    }


    memset(buffer, 0, sizeof(buffer));
    if (read(sock, buffer, sizeof(buffer)) < 0) {
        perror("read");
        close(sock);
        exit(1);
    }
    printf("%s\n", buffer);

    
    if (write(sock, argus->voteArguments[2], strlen(argus->voteArguments[2])+1) < 0)
    {
        perror("write");
        close(sock);
        exit(1);
    }

    memset(buffer, 0, sizeof(buffer));
    if (read(sock, buffer, sizeof(buffer)) < 0) {
        perror("read");
        close(sock);
        exit(1);
    }
    printf("%s\n", buffer);
 
  
    close(sock);
}

void* operateVote(void *argg) 
{
    struct arguments *argus = (struct arguments*) argg;

    sleep(1); 

    pthread_mutex_lock(&mutexBuff);
    while (!ready) {
        pthread_cond_wait(&cond, &mutexBuff);
    }
    pthread_mutex_unlock(&mutexBuff);

    sendVote(argus);

    free(argus->voteArguments);

    return 0;
}

int main(int argc, char *argv[])
{
    
    struct hostent *serverName;
    if(( serverName = gethostbyname(argv[1])) == NULL)
    {
        herror("gethostbyname");
        exit(1);
    }

   
    int portNum = atoi(argv[2]);
    char *inputFile = argv[3];

    if (argc != 4)
    {
        printf("usage %s , PLEASE give  serverName , portNum and inputFile.txt\n",argv[0]);
        exit(1);
    }

    pthread_t threads[1024];
    FILE *file = fopen(inputFile, "r"); //opens file for reading
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", inputFile);
        return 1;
    }

    int counterThread = 0;
    char line[1024];
    while (fgets(line,sizeof(line),file) != NULL) //read each line
    {
        //checks
        if (strlen(line) == 0 )
        {
            break;
        }

        int lineLength = strlen(line);

        if (line[lineLength - 1] == '\n')
        {
            line[lineLength - 1] = '\0';
        }

        //tokenization of the args
        char **voteArguments = (char **)malloc(3 *sizeof(char*));
        char *token = strtok(line," ");

        struct arguments *ar;
        ar= (struct arguments*)malloc(sizeof(struct arguments));
        ar->serverName = serverName;
        ar->portNum = portNum;
        ar->file = file;
        ar->voteArguments = voteArguments;

        //px kostas merkouris kommaA
        //pairnei ksexwrista ta tria tokens kai ta exei sto token array
        for (int i = 0; i < 3; i++)
        {
            ar->voteArguments[i] = strdup(token);//antigrafei duplicate ta args
            token = strtok(NULL, " ");//pairnei to epomeno token
        }


        if (pthread_create(&threads[counterThread],NULL, operateVote, (void *)ar))
        {
            printf("create!");
            exit(1);
        }
        counterThread++;

    }

    // Wait for all threads to start before signaling
    
    sleep(1);
    pthread_mutex_lock(&mutexBuff);
    ready = 1;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutexBuff);


    for (int i = 0; i < counterThread; i++) 
    {
        if(pthread_join(threads[i], NULL))
        {
            printf("join cc\n");
            exit(1);
        }
    }

    fclose(file);
    return 0;

}
