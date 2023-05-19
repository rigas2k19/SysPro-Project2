#include <stdio.h>
#include <sys/types.h> /* sockets */
#include <sys/socket.h> /* sockets */
#include <netinet/in.h> /* internet sockets */
#include <unistd.h> /* read, write, close */
#include <netdb.h> /* gethostbyaddr */
#include <stdlib.h> /* exit */
#include <string.h> /* strlen */
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include "include/myDeclarations.h"

#define BUFFSIZE 512

void perror_exit(char *message);

void main(int argc, char *argv[]) {
    int port, sock, i;
    
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*)&server;
    struct hostent *rem;
    
    if(argc != 7) {
        printf("Please give all arguments\n");
        exit(1);
    }
    
    char* name, *directory;
    for(int j=1; j<argc; j++){
        if(strcmp(argv[j], "-i") == 0){
            name = malloc(strlen(argv[j+1]) + 1);
            strcpy(name, argv[j+1]);
        }
        else if(strcmp(argv[j], "-p") == 0){
            /* Convert port number to integer */
            port = atoi(argv[j+1]);
        }
        else if(strcmp(argv[j], "-d") == 0){
            /* Get directory */
            directory = malloc(strlen(argv[j+1]) + 1);
            strcpy(directory, argv[j+1]);
            /* If given directory ends in '/' we remove it */
            if(directory[strlen(directory)-1] == '/'){
                directory[strlen(directory)-1] = '\0';
            }
        }
    }

    /* Create socket */
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror_exit("socket");
    }

    /* Find server address */
    if((rem = gethostbyname(name)) == NULL) {
        herror("gethostbyname"); exit(1);
    } 
    
    server.sin_family = AF_INET; /* Internet domain */

    /* Write on server the addresses we found */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    
    server.sin_port = htons(port); /* Server port */
    
    /* Initiate connection */
    if(connect(sock, serverptr, sizeof(server)) < 0){
        perror_exit("connect");
    }
        
    char* pos, *copyFilepath, *cutFilepath;
    char* read_file, *eof;
    int bytes, bytes_counter = 0;
    int write_fd, position, len;
    /***********************************************************/
    /*     Should get path like "home/user/CopyFolder"         */
    /*     and i should split it for CopyFolder                */
    /***********************************************************/
    printf("\nConnecting to %s port %d\n\n", name, port);
    
    /* Send directory */
    if(write(sock, directory, strlen(directory)+1) < 0){
        perror_exit("Write");
    }

    /* Get number of files */
    int number_of_files_received = 0;
    if(read(sock, &number_of_files_received, sizeof(number_of_files_received)) < 0){
        perror_exit("Read");
    }
    printf("Client reads %d files \n\n", ntohl(number_of_files_received));

    char* ptr, *reply, *token, *path, *fptr, *eof_read;
    const char* ret;
    int j=0, fsize, size, eof_size, ncounter, byte, received = htonl(1), end = htonl(2);
    for(int k=0; k<ntohl(number_of_files_received); k++){
        /* Get file metadata (size) */
        fsize = 0;
        if(read(sock, &fsize, sizeof(fsize)) < 0){
            perror_exit("Read");
        }
        size = 0;
        size = ntohl(fsize);
        
        /* Read filepath */
        ncounter = 0;
        reply = malloc(BUFFSIZE);
        token = malloc(BUFFSIZE);
        path = malloc(BUFFSIZE);
        path[0] = '\0';
        reply[0] = '\0';
        while((byte = read(sock, reply, BUFFSIZE)) > 0){
            /* First thing to be passed is file path */
            if(ncounter == 0){
                strcat(path, reply);
                /* If there is a '\n' we got our file path */
                ptr = strchr(path, '\n');
                if(ptr != NULL){
                    ncounter++;
                    path[strlen(path) - strlen(ptr)] = '\0';
                    
                    /* Send message to server */
                    if(write(sock, &received, sizeof(received)) < 0){
                        perror_exit("Write");
                    }
                    break;
                }
            }
        }
        printf("Path received is %s\n", path);

        /* Remove unwanted folders from our filepath */
        cutFilepath = malloc(strlen(directory)+1);
        strcpy(cutFilepath, directory);
        
        fptr = strrchr(directory, '/');
        if(fptr != NULL){
            len = strlen(cutFilepath);
            position = len - strlen(fptr);
            memmove(cutFilepath, cutFilepath + position + 1, len - position + 1);
        }

        /* Find first occurence of folder in path */
        ret = strstr(path, cutFilepath);
        createFolders(ret);

        /* Build copy paths */
        copyFilepath = malloc(strlen(ret) + 10);
        copyFilepath[0] = '\0';
        strcat(copyFilepath, "./Client/");
        strcat(copyFilepath, ret);

        /* Build eof */
        eof = malloc(strlen(path) + strlen("-EOF") + 1);
        eof[0] = '\0';
        strcat(eof, path);
        strcat(eof, "-EOF");
        eof_size = strlen(eof);
        
        read_file = malloc(size);

        if((write_fd = open(copyFilepath, O_CREAT|O_WRONLY, 0777)) == -1){
            perror_exit("Open");
        }

        /* Read file */
        while((byte = read(sock, read_file, size)) > 0){
            /* If we reached the end of file */
            if(strcmp(read_file, eof) == 0){
                break;
            }
            /* Write on file */
            if(write(write_fd, read_file, strlen(read_file)) < 0){
                perror_exit("Write");
            }
            /* Send message to server */
            if(write(sock, &received, sizeof(received)) < 0){
                perror_exit("Write");
            }
        }
        close(write_fd);

        printf("Finished with file %s\n\n", copyFilepath);
        
        /* If its the last file we send 2 */
        if(k == ntohl(number_of_files_received)-1){
            /* Send message to server */
            if(write(sock, &end, sizeof(end)) < 0){
                perror_exit("Write");
            }
        }
        /* If not we send 1 */
        else{
            /* Send message to server */
            if(write(sock, &received, sizeof(received)) < 0){
                perror_exit("Write");
            }
        }
        
        free(reply);
        free(token);
        free(path);
        free(copyFilepath);
        free(eof);
        free(read_file);
    }

    free(directory);
    free(name);

    printf("Client exits \n");
    close(sock); /* Close socket and exit */
}

void createFolders(const char* filepath){
    char* token;
    int counter = 0;

    char* path = malloc(strlen(filepath)+1);
    strcpy(path, filepath);
    
    for(int i=0; i<strlen(path); i++){
        if(path[i] == '/'){
            counter++;
        }
    }
    if(path[0] == '/'){
        counter--;
    }
    
    char* my_path = malloc(strlen(path) + counter + 12);
    my_path[0] = '\0';
    strcat(my_path, "./Client/");

    char* filename;

    /* Creates folder client */
    int check = mkdir("./Client", 0777);
    if(!check){
        printf("Client directory created\n");
    }
    /* If error is other than already exists */
    else if(errno != EEXIST){
        printf("Unable to create directory Client\n");
        exit(1);
    }


    struct stat sb;
    /* Split the string by '/' */
    FILE* fp = NULL;
    int token_counter = 0, dir;
    token = strtok(path, "/");
    while(token != NULL){
        if(token_counter < counter){
            strcat(my_path, token);
            
            dir = mkdir(my_path, 0777);
            if(!dir){
                printf("Directory %s created\n", my_path);
            }
            /* If error is other than already exists */
            else if(errno != EEXIST){
                printf("Unable to create directory %s\n", my_path);
                exit(1);
            }

            strcat(my_path, "/");
            
        }
        else{
            strcat(my_path, token);

            if(stat(my_path, &sb) == 0){
                if(remove(my_path) != 0){
                    perror_exit("Cannot delete file");
                }
                else{
                    printf("File deleted\n");
                }
            }
            
            printf("Creating %s \n", my_path);
            

            /* Create file */
            fp = fopen(my_path, "a");
            if(fp == NULL){
                printf("Unable to create file \n");
                exit(1);
            }
            
            fclose(fp);
        }
        token_counter++;
        token = strtok(NULL, "/");
    }
    free(path);
    free(my_path);
}

void perror_exit(char *message){
    perror(message);
    exit(EXIT_FAILURE);
}