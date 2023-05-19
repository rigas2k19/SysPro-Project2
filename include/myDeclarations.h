/* Struct for elements to be passed by communication thread to worker */
typedef struct addVars* AddVars;
struct addVars{
    char* path;
    int socket_id;
    int file_size;
};

void* child_server(void* newsock);

void perror_exit(char *message);

void get_files(const char* directory, const void* socket);

void* workerThread(void* args);

void createFolders(const char* filepath);

void get_number_of_files(const char* directory, const void* newsock, int* file_counter);
