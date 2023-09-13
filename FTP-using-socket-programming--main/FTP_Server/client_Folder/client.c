// gcc client.c -o client
// ./client <ip_add> <port>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include <sys/sendfile.h>
#include <fcntl.h>

int socket_fd = 0, n = 0;
char choice_str[1000];
int choice;
int filehandle;
char filename[20], buf[100], ext[20], command[20];
struct stat obj;
FILE *fp;
char *f;
int size, status, i = 1;
int overwirte_choice = 1;
int already_exits = 0;
int num_lines;
char *pos;

void putInFileServer()
{
    printf("enter the filename to put in server\n");
    scanf("%s", filename);          // read the file name
    if (access(filename, F_OK) < 0) // checking if a file exists in the local directory or not
    {
        printf(" %s : No such File found!!!! \n", filename);
        return;
    }
    strcpy(buf, "put ");                   // copying the command
    filehandle = open(filename, O_RDONLY); // open the file in read only mode
    strcat(buf, filename);
    write(socket_fd, buf, 100); // send put command with filename
    printf("\nSending the File to Server...\n");
    // reading the status if the file already exists in the server or not
    read(socket_fd, &already_exits, sizeof(int));
    if (already_exits != 0)
    {
        printf("same name file already exits in server\n"); // file is already exits
        printf("1. overwirte\n2.NO overwirte\n");
        scanf("%d", &overwirte_choice);
    }
    // by default we overwrite the file
    write(socket_fd, &overwirte_choice, sizeof(int)); // sending overwrite choice

    if (overwirte_choice == 1)
    {
        // stat to find the deatils like size, mode of the file
        stat(filename, &obj);
        printf("\nWriting the file to server\n");
        size = obj.st_size;
        // sending the size of the file to the server
        write(socket_fd, &size, sizeof(int));
        printf("\nOverwriting the File into Server...\n");
        sendfile(socket_fd, filehandle, NULL, size); // sending file
        read(socket_fd, &status, sizeof(int));       // receiving the status
        if (status != 0)
            printf("%s File stored successfully\n", filename); // status
        else
            printf("%s File failed to be stored to remote machine\n", filename);
    }
}

void getInFileServer()
{
    printf("Enter filename to retrieve(GET) from the Server: ");
    scanf("%s", filename);
    strcpy(buf, "get ");
    strcat(buf, filename);
    printf("\nSending request to Server to retrieve the file\n");
    write(socket_fd, buf, 100);          // send the get command with file name
    read(socket_fd, &size, sizeof(int)); // receving the size of the file
    if (!size)
    {
        printf("No such file on the remote directory\n\n"); // file doesn't exits
        return;
    }

    if (access(filename, F_OK) != -1) // checking existence of file at client side
    {
        printf("same name file already exits in Client\n"); // file is already exits
        printf("1. overwirte\n2.NO overwirte\n");           // file already exits
        already_exits = 1;
        scanf("%d", &overwirte_choice);
    }
    // send the overwrite choice by default 1
    write(socket_fd, &overwirte_choice, sizeof(int));

    if (already_exits && overwirte_choice == 1)
    {
        filehandle = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 644); // open file with all clear data
        printf("\nYou have chosen to overwrite the file\n");
        f = malloc(size);
        read(socket_fd, f, size);
        write(filehandle, f, size);
        close(filehandle);
    }
    else if (already_exits == 0 && overwirte_choice == 1)
    {
        filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666); // open new file
        f = malloc(size);
        read(socket_fd, f, size);
        write(filehandle, f, size);
        close(filehandle);
    }
}

void mputInFileServer()
{
    printf("Enter the extension , you want to put in server:\n");
    printf("\nEnter the extension without any dot at the start\n");
    scanf("%s", ext); //  take the extionsion
    printf("\nFetching all the files..\n");
    strcpy(command, "ls *.");
    strcat(command, ext);
    strcat(command, " > temp.txt"); // store all file list in temp.txt
    // printf("%s\n",command);
    system(command);

    size_t len = 0;
    char *line = NULL; // intilize file var
    ssize_t read;
    FILE *fp = fopen("temp.txt", "r");
    while ((read = getline(&line, &len, fp)) != -1) // read input
    {
        // removing the \n charater and replace with null character to make the file name string
        if ((pos = strchr(line, '\n')) != NULL)
            *pos = '\0';

        strcpy(buf, "put ");
        filehandle = open(line, O_RDONLY); // open files line wise
        strcat(buf, line);

        write(socket_fd, buf, 100);
        recv(socket_fd, &already_exits, sizeof(int), 0);
        if (already_exits == 1)
        {
            printf("same name file already exits in server\n"); // file is already exits
            printf("1. overwirte\n2.NO overwirte\n");           // overwrite option for that particular file
            scanf("%d", &overwirte_choice);
        }
        write(socket_fd, &overwirte_choice, sizeof(int)); // sending overwrite choice
        if (overwirte_choice == 1)
        {
            stat(line, &obj);
            printf("\nYou have chosen to overwrite the file\n");
            size = obj.st_size;
            write(socket_fd, &size, sizeof(int));
            sendfile(socket_fd, filehandle, NULL, size);
            recv(socket_fd, &status, sizeof(int), 0);
            if (status != 0) // status
                printf("%s stored successfully\n", line);
            else
                printf("%s failed to be stored to remote machine\n", line);
        }
        overwirte_choice = 1; // re-assign overwrite choice
    }                         // end of while
    fclose(fp);               // close the file
    remove("temp.txt");
}

void mgetInFileServer()
{
    printf("enter the extension , you want to get from server:\n");
    printf("\nEnter the extension without any dot at the start\n");
    scanf("%s", ext); // take input the files extension
    strcpy(buf, "mget ");
    strcat(buf, ext);
    write(socket_fd, buf, 100);                  // sending buffer with choice and extension
    recv(socket_fd, &num_lines, sizeof(int), 0); // get number of files

    while (num_lines > 0)
    {

        recv(socket_fd, filename, 20, 0); // recv file name
        printf("\n");
        recv(socket_fd, &size, sizeof(int), 0); // recv the size of file
        if (!size)
        {
            printf("No such file on the remote directory\n\n"); // error handling
            break;
        }

        if (access(filename, F_OK) != -1) // checking if already exits or not
        {

            printf("%s file already exits in client 1. overwirte 2.NO overwirte\n", filename);
            already_exits = 1;
            scanf("%d", &overwirte_choice); // taking overwirte choice
        }
        write(socket_fd, &overwirte_choice, sizeof(int)); // sending overwrite choice

        if (already_exits && overwirte_choice == 1) // option according to the choice
        {
            filehandle = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 644); // clear all the file
            f = malloc(size);
            recv(socket_fd, f, size, 0);
            write(filehandle, f, size); // send file
            close(filehandle);
        }
        else if (!already_exits && overwirte_choice == 1)
        {
            filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666); // open new file
            f = malloc(size);
            recv(socket_fd, f, size, 0);
            write(filehandle, f, size);
            close(filehandle);
        }
        num_lines--;
        already_exits = 0;
        overwirte_choice = 1;
    }
}

void removeConnection()
{
    strcpy(buf, "quit");
    write(socket_fd, buf, 100); // sending quit command for closing both server and client
    recv(socket_fd, &status, 100, 0);
    if (status != 0)
    {
        printf("Server closed\n");
        printf("\nQuitting...\n");
        exit(0);
    }
    printf("Server failed to close connection\n"); // faild to quit
    return;
}

int main(int argc, char *argv[])
{
    // char recvBuff[1024];
    struct sockaddr_in server_address;

    if (argc != 3)
    {
        printf("\n proper ip and port required!!!\n");
        printf("\n Usage: %s <ip of server> \n", argv[0]); // checking argument
        return 1;
    }

    // memset(recvBuff, '0', sizeof(recvBuff));
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Client socket creation failed \n"); // socket create
        return 0;
    }

    // memset(&server_address, '0', sizeof(server_address)); // assigning  server address

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[2])); // assigning port

    if (inet_pton(AF_INET, argv[1], &server_address.sin_addr) <= 0)
    {
        printf("\n inet_pton error occured\n");
        return 0;
    }

    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) // connect to the server
    {
        printf("\n Can't connect with server!!! \n");
        return 1;
    }

    while (1)
    {
        printf("Enter a choice:\n1- put\n2- get\n 3- mput\n 4-mget\n 5-quit\n");

        fgets(choice_str, 1000, stdin);

        choice_str[strlen(choice_str) - 1] = '\0';
        if (strlen(choice_str) > 1)
        {
            printf("error\n");
            continue;
        }
        choice = atoi(choice_str);
        // scanf("%d", &choice);

        switch (choice)
        {

            //--------put file in server----------------//
        case 1:
            putInFileServer();
            break;
            //----------------get file from server-------------------//
        case 2:
            getInFileServer();
            break;
            //------------------quit the server-------------------------//
        case 5:
            removeConnection();
            //------------------mput server ----------------------------------//
        case 3:
            mputInFileServer();
            break;
            //----------------mget server------------------------------------//
        case 4:
            mgetInFileServer();
            break;

            //------------- default choice --------------------//
        default:
            printf("choose the vaild option\n");
            break;

        } // end of switch

    } // end of while

    return 0;
} // end of main