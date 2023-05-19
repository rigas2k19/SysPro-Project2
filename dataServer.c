#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h> /* sockets */
#include <sys/types.h> /* sockets */
#include <sys/socket.h> /* sockets */
#include <netinet/in.h> /* internet sockets */
#include <netdb.h> /* gethostbyaddr */
#include <unistd.h> /* fork */
#include <stdlib.h> /* exit */
#include <ctype.h> /* toupper */
#include <signal.h> /* signal */
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include "include/MyQueue.h"

Queue execution_queue;

/* Mutex and condition variable for file queue */
pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;

/* Mutex for writing in socket */
pthread_mutex_t mutexWrite;

void main(int argc, char* argv[]){
    int port, sock, newsock, queue_size, thread_pool_size, block_size;
    
    if (argc != 9) {
        printf("Please give all arguments ./dataServer -p <port> -s <thread_pool_size> -q <queue_size> -b <vlock_size>\n");
        exit(1);
    }

    for(int i=1; i<argc; i++){
        if(strcmp(argv[i], "-p") == 0){
            port = atoi(argv[i+1]);
        }
        else if(strcmp(argv[i], "-s") == 0){
            thread_pool_size = atoi(argv[i+1]); 
        }
        else if(strcmp(argv[i], "-q") == 0){
            queue_size = atoi(argv[i+1]);
        }
        else if(strcmp(argv[i], "-b") == 0){
            block_size = atoi(argv[i+1]);
        }
    }

    printf("Server's parameters are:\n");
    printf("port: %d\nthread_pool_size: %d\nqueue_size: %d\nblock_size: %d\n", port, thread_pool_size, queue_size, block_size);
    printf("\nServer was succesfully initialized\n\n");

    struct sockaddr_in server, client;
    socklen_t clientlen;
    struct sockaddr *serverptr=(struct sockaddr *)&server;
    struct sockaddr *clientptr=(struct sockaddr *)&client;
    struct hostent *rem;

    /* Create mutex */
    pthread_mutex_init(&mutexQueue, 0);
    pthread_mutex_init(&mutexWrite, 0);
    
    /* Create condition variables */
    pthread_cond_init(&condQueue, 0);

    execution_queue = q_create(free, queue_size);

    /* Create <thread_pool_size> worker threads */
    pthread_t worker_threads[thread_pool_size];
    for(int i=0; i<thread_pool_size; i++){
        if(pthread_create(&worker_threads[i], NULL, workerThread, &block_size) != 0){
            perror("Worker Thread Create");
            exit(2);
        }
    }

    /* Create socket */
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror_exit("socket");
    }
    
    server.sin_family = AF_INET; /* Internet domain */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port); /* The given port */
    
    /* Bind socket to address */
    if(bind(sock, serverptr, sizeof(server)) < 0){
        perror_exit("bind");
    }
    
    /* Listen for connections */
    if(listen(sock, 1000) < 0){
        perror_exit("listen");
    }

    printf("\nListening for connections to port %d\n\n", port);
    while(1){
        clientlen = sizeof(client);
        
        /* Accept connection - Blocks until connection is made */
        if((newsock = accept(sock, clientptr, &clientlen))< 0){
            perror_exit("accept");
        }
        
        /* Find client's name after we got a connection */
        if((rem = gethostbyaddr((char *) &client.sin_addr.s_addr,sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL){
            herror("gethostbyaddr");
            exit(1);
        }

        printf("Accepted connection from %s\n", rem->h_name);
        
        /* Create Communication Thread */
        pthread_t comms_thread;
        int* pclient = malloc(sizeof(int));
        pclient = &newsock;
        pthread_create(&comms_thread, NULL, child_server, pclient);
        
    }
}

/* Server communication thread function that reads directory name */
void* child_server(void* newsock){
    char buf[256];
    
    /* Perhaps a mutex ? */
    int tmp=0;
    int file_counter = 0;
    
    while(read(*(int*)newsock, buf, 256) > 0) {
        if(strlen(buf) > 0){
            printf("[Thread: %ld]: About to scan directory %s \n", pthread_self(), buf);
        }
        get_number_of_files(buf, newsock, &file_counter);
        
        /* Pass number of files to be read */
        tmp = htonl(file_counter);
        if(write(*(int*)newsock, &tmp, sizeof(tmp)) < 0){
            perror_exit("Write");
        }

        get_files(buf, newsock);
    }

}

void get_number_of_files(const char* directory, const void* newsock, int* file_counter){
    DIR* dir;
    
    dir = opendir(directory);
    /* Check if directory exists */
    if(dir == NULL){
        return;
    }

    /* We are going to use this pointer to iterate over all the 
    elements in directory */
    struct dirent* entity;

    char* apath = malloc(512);
    entity = readdir(dir);
    while(entity != NULL){
        /* Create path recursively */
        if(entity->d_type == DT_DIR && strcmp(entity->d_name, ".") != 0 && strcmp(entity->d_name, "..") != 0){
            char path[200] = { 0 };
            strcat(path,directory);
            strcat(path, "/");
            strcat(path, entity->d_name);
            
            strcat(apath, path);
            
            get_number_of_files(path, newsock, file_counter);
        }
        
        if(entity->d_type == DT_REG){
            (*file_counter)++;
        }

        entity = readdir(dir);
    }
    closedir(dir);

    free(apath);
}

/* Function that reads directory recursively placing files in queue */
void get_files(const char* directory, const void* sock){
    DIR* dir;
    
    /* Now we should print directory contents recursively */ 
    dir = opendir(directory);

    /* Check if directory exists */
    if(dir == NULL){
        return;
    }

    /* We are going to use this pointer to iterate over all the 
    elements in directory */
    struct dirent* entity;
    struct stat st; /* To get size of file */

    AddVars av;
    char* apath = malloc(512);
    entity = readdir(dir);
    while(entity != NULL){
        /* Create path recursively */
        if(entity->d_type == DT_DIR && strcmp(entity->d_name, ".") != 0 && strcmp(entity->d_name, "..") != 0){
            char path[200] = { 0 };
            strcat(path,directory);
            strcat(path, "/");
            strcat(path, entity->d_name);
            
            strcat(apath, path);
            
            get_files(path, sock);
        }

        if(strcmp((char*)entity->d_name, ".") != 0 && strcmp((char*)entity->d_name, "..") != 0){
            /* Add file path to queue */
            pthread_mutex_lock(&mutexQueue);

            while(q_size(execution_queue) == q_maxsize(execution_queue)){
                printf(">> Queue is max size %d. Waiting...\n", q_size(execution_queue));
                pthread_cond_wait(&condQueue, &mutexQueue);
            }

            /* Queue is not full so we add file path */
            apath[0] = '\0';
            strcat(apath, directory);
            if(entity->d_type == DT_REG){
                strcat(apath, "/");
                strcat(apath, (char*)entity->d_name);
            }
            
            stat(apath, &st);
            av = malloc(sizeof(*av));
            av->file_size = st.st_size;
            av->path = malloc(strlen(apath)+1);
            strcpy(av->path, apath);
            av->socket_id = *(int*)sock;

            /* I only pass file paths to queue */
            if(entity->d_type == DT_REG){
                printf("[Thread: %ld]: Adding file %s to queue\n", pthread_self(), av->path);

                q_insert(execution_queue, av);
            
            }
            
            
            /* Since we copy the file we dont need the apath */
            memset(apath, '\0', strlen(apath));
            
            pthread_cond_signal(&condQueue);
            
            pthread_mutex_unlock(&mutexQueue);
        }
        entity = readdir(dir);
    }
    closedir(dir);
    
    free(apath);
}

/* Worker thread function that is going to wait until it gets a filename 
from queue to read */
void* workerThread(void* args){
    int length, block_length = *(int*)args, size, received = ntohl(0);
    char* filename, *metadata, *block, *p_path;
    char* buf;
    AddVars buff;
    char file_read[block_length];
    FILE* fptr; /* To open and read file */
    while(1){
        /* If queue is empty, wait */
        pthread_mutex_lock(&mutexQueue);

        while(q_size(execution_queue) == 0){
            printf(">> Queue is empty. Waiting...\n");
            pthread_cond_wait(&condQueue, &mutexQueue);
        }
        
        /* Now that the queue has at least one element we get the first struct object */
        
        buff = q_first_value(execution_queue);
        
        printf("\n\n[Thread: %ld]: Received task %s with socketId : %d and size : %d our block size is: %d\n", pthread_self(), buff->path, buff->socket_id, buff->file_size, *(int*)args);

        /* Only one thread should be writting in socket */
        pthread_mutex_lock(&mutexWrite);

        /* Pass file metadata (file size) */
        size = htonl(buff->file_size); // file size + path + path-EOF + \n
        if(write(buff->socket_id, &size, sizeof(size)) < 0){
            perror_exit("Write");
        }
        
        /* Pass file path */
        p_path = malloc(strlen(buff->path) + 2);
        strcpy(p_path, buff->path);
        strcat(p_path, "\n");
        if(write(buff->socket_id, p_path, strlen(p_path)) < 0){
            perror_exit("Write");
        }
        /* Get message from client */
        if(read(buff->socket_id, &received, sizeof(received)) < 0){
            perror_exit("Read");
        }
        if(ntohl(received) == 1){
            received = 0;
        }

        /* Worker opens file and sends it to client block by block */
        struct stat path_stat;
        stat(buff->path, &path_stat); /* To find out if its a regfile */

        /* To find the end of file */
        char* eof = malloc(strlen(buff->path) + 5); // "-EOF"
        eof[0] = '\0';
        strcpy(eof, buff->path);
        strcat(eof, "-EOF");

        /* If it is a regular file we open and send it block by block */
        if(S_ISREG(path_stat.st_mode)){
            fptr = fopen(buff->path, "r");
            if(fptr == NULL){
                perror_exit("Can't open file");
            }
            while(fgets(file_read, block_length, fptr) != NULL){
                /* Write in client */
                if(write(buff->socket_id, file_read, block_length) < 0){
                    perror_exit("Write");
                }
                /* Get message from client */
                if(read(buff->socket_id, &received, sizeof(received)) < 0){
                    perror_exit("Read");
                }
                if(ntohl(received) == 1){
                    received = 0;
                }
            }
            
            /* Pass the end of file */
            if(write(buff->socket_id, eof, block_length) < 0){
                perror_exit("Write");
            }
            /* Get message from client */
            if(read(buff->socket_id, &received, sizeof(received)) < 0){
                perror_exit("Read");
            }
            if(ntohl(received) == 1){
                received = 0;
            }
            else if(ntohl(received) == 2){
                received = 0;
                printf("Closing socket \n");
                /* Close socket */
                close(buff->socket_id);
            }
            
            fclose(fptr);
        }
        
        pthread_mutex_unlock(&mutexWrite);

        free(eof);
        free(p_path);

        q_remove_first(execution_queue);

        /* Send signal to communication thread */
        pthread_cond_signal(&condQueue);

        pthread_mutex_unlock(&mutexQueue);
    }
}

void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}