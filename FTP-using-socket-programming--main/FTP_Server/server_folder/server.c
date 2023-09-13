// gcc server.c -o server
// ./server <port>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>

/*for getting file size using stat()*/
#include <sys/stat.h>

/*for sendfile()*/
#include <sys/sendfile.h>

char buf[100], command[5], filename[20], ext[20], lscommand[20]; // defining variables
int listenfd = 0, connfd = 0;
struct stat obj;
int size, i, filehandle;
int overwrite_choice = 1;
int already_exits = 0;

char *pos;

void putFileFromClient()
{
    char *f;
    int c = 0, len;
    // stlen is used here as offset
    // sscanf stores the filename from the buf into filename
    sscanf(buf + strlen(command), "%s", filename); // store filename in var
    i = 1;
    // check file already exits or not
    printf("\nChecking if File already exist in the server or not\n");
    if (access(filename, F_OK) != -1)
    {
        printf("\nFile already exists in the Server\n");
        already_exits = 1;
        write(connfd, &already_exits, sizeof(int)); // exits
    }
    else
    {
        printf("File doesn't exist in the server. Putting file in the server...\n");
        already_exits = 0;
        write(connfd, &already_exits, sizeof(int)); // not exits
    }
    // it receives the command to either overwrite or not
    recv(connfd, &overwrite_choice, sizeof(int), 0); // recv overwrite choice

    // case of overwrite
    if (already_exits && overwrite_choice == 1)
    {
        // As the file already existis clear all the data
        filehandle = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 644); // clear all the file data
        // recevies the file size from client
        read(connfd, &size, sizeof(int));
        // Allocating the buffer for reading the file
        f = malloc(size);
        read(connfd, f, size);          // recv full file data
        c = write(filehandle, f, size); // write data in file
        close(filehandle);              // close file
        write(connfd, &c, sizeof(int)); // send status
    }
    else if (already_exits == 0 && overwrite_choice == 1) // creating the new file
    {
        filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666); // open file
        read(connfd, &size, sizeof(int));
        f = malloc(size);
        read(connfd, f, size);
        c = write(filehandle, f, size);
        close(filehandle);
        write(connfd, &c, sizeof(int));
    }
}

void getFileFromClient()
{
    sscanf(buf, "%s%s", filename, filename);
    printf("Reading File Name..\n");
    filehandle = open(filename, O_RDONLY); // open file with read only option
    stat(filename, &obj);                  // details of the file
    size = obj.st_size;
    if (filehandle == -1) // if file doesnt exist
        size = 0;
    write(connfd, &size, sizeof(int)); // sending the size of file
    if (size <= 0)
        return;
    read(connfd, &overwrite_choice, sizeof(int)); // recv over write choice
    if (size && overwrite_choice == 1)
    {
        printf("Sending file to the Client...\n");
        sendfile(connfd, filehandle, NULL, size); // sending the file
    }
}

void mgetFileFromClient()
{
    sscanf(buf, "%s%s", ext, ext); // get the extension
    printf("%s\n", ext);
    strcpy(lscommand, "ls *.");
    strcat(lscommand, ext);
    printf("\nFetching all the files..\n");
    strcat(lscommand, "> filelist.txt"); // run the command and store the filelist in filelist.txt
    system(lscommand);

    size_t len = 0;
    char *line = NULL;
    ssize_t read;

    FILE *fp = fopen("filelist.txt", "r"); // open the list of files
    int ch;
    int num_lines = 0;
    while (!feof(fp)) // count the number of files
    {
        ch = fgetc(fp);
        if (ch == '\n')
        {
            num_lines++;
        }
    } // end of while

    printf("%d\n", num_lines);
    fclose(fp);                             // closing
    fp = fopen("filelist.txt", "r");        // reopen the file list
    write(connfd, &num_lines, sizeof(int)); // sending the number of files

    while ((read = getline(&line, &len, fp)) != -1) // sending all files in while loop
    {
        if ((pos = strchr(line, '\n')) != NULL)
            *pos = '\0';
        strcpy(filename, line);
        write(connfd, filename, 20); // sending the command

        filehandle = open(line, O_RDONLY); // open the file only read choice
        stat(line, &obj);
        size = obj.st_size;
        if (filehandle == -1)
            size = 0;
        write(connfd, &size, sizeof(int));               // send the size
        recv(connfd, &overwrite_choice, sizeof(int), 0); // recv overwrite choice
        printf("\n");
        if (size && overwrite_choice == 1)
            sendfile(connfd, filehandle, NULL, size); // finally send the file

    } // end of while loop

    fclose(fp);
    remove("filelist.txt");
}

void quitServer()
{
    printf("FTP server quitting..\n");
    i = 1;
    write(connfd, &i, sizeof(int)); // closing the server
    exit(0);
}

int main(int argc, char *argv[])
{

    struct sockaddr_in serv_addr, client;

    listenfd = socket(AF_INET, SOCK_STREAM, 0); // create socket

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));     // port which in input
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // ip address
    printf("Binding...\n");
    bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    printf("Binding Done...\n");
    printf("Listening...\n");
    listen(listenfd, 10);

    connfd = accept(listenfd, (struct sockaddr *)NULL, NULL); // accept the connection of client
    printf("connected to the client\n");

    // while loop start for all opreations
    while (1)
    {

        recv(connfd, buf, 100, 0);
        sscanf(buf, "%s", command); // get command with file option
                                    //----------- for put command -------------------------//

        if (!strcmp(command, "put"))
        {
            putFileFromClient();

        } // ending of put option

        //------------------get option ----------------------------//
        else if (!strcmp(command, "get"))
        {
            getFileFromClient();

        } // ending the get option

        //--------------------quit command----------------------------------------//
        else if (!strcmp(command, "quit"))
        {
            quitServer();
        } // ending of quit option

        //---------------------mget option ---------------------------------//
        else if (!strcmp(command, "mget"))
        {
            mgetFileFromClient();

        } // end of mget option

    } // end of outer while loop
} // end of main function