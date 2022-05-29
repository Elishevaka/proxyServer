#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "threadpool.h"
#define INVALID 0

#define error_400 "<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n<BODY><H4>400 Bad request</H4>\nBad Request.\n</BODY></HTML>\n"
#define error_501 "<HTML><HEAD><TITLE>501 Not supported</TITLE></HEAD>\n<BODY><H4>501 Not supported</H4>\nMethod is not supported.\n</BODY></HTML>\n"
#define error_500 "<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n<BODY><H4>500 Internal Server Error</H4>\nSome server side error.\n</BODY></HTML>\n"
#define error_404 "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n<BODY><H4>404 Not Found</H4>\nFile not found.\n</BODY></HTML>\n"
#define error_403 "<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n<BODY><H4>403 Forbidden</H4>\nAccess denied.\n</BODY></HTML>\n"
#define BUFLEN 1000
char** array;
int line_count;
char *pathFilter;

////--------------------------------------------------------------------------------------------------------------------
void error_num(int num, int serving_socket){
    char* string1;
    int len;
    if(num == 400){
        string1 = "Bad Request";
        len = 113;
    }
    if(num == 404) {
        string1 = "Not Found";
        len = 112;
    }
    if(num == 403) {
        string1 = "Forbidden";
        len = 111;
    }
    if(num == 500) {
        string1 = "Internal Server Error";
        len = 144;
    }
    if(num == 501) {
        string1 = "Not supported";
        len = 129;
    }
    char *str = malloc(sizeof (char)*len+4+77+strlen(string1));

    sprintf(str, "HTTP/1.0 %d %s\n"//22
            "Content-Type: text/html\n"//24
            "Content-Length: %d\n"//17
            "Connection: close\r\n\r\n", num, string1, len );//19+3+3

    write(serving_socket, str, strlen(str));
    if(num == 400){
        write(serving_socket, error_400, strlen(error_400));
//        printf("%s", error_400);
    }
    if(num == 404){
        write(serving_socket, error_404, strlen(error_404));
//        printf("%s", error_404);
    }
    if(num == 403){
        write(serving_socket, error_403, strlen(error_403));
//        printf("%s", error_403);
    }
    if(num == 500){
        write(serving_socket, error_500, strlen(error_500));
//        printf("%s", error_500);
    }
    if(num == 501){
        write(serving_socket, error_501, strlen(error_501));
//        printf("%s", error_501);
    }
    free(str);
}
////--------------------------------------------------------------------------------------------------------------------
char *get_mime_type(char *name)
{
    char *ext = strrchr(name, '.');
    if (!ext) return NULL;
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".au") == 0) return "audio/basic";
    if (strcmp(ext, ".wav") == 0) return "audio/wav";
    if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
    if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
    if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
    return NULL;
}
////=====================================================================================================================
int isFile(const char* name){
    DIR* directory = opendir(name);
    if(directory != NULL)
    {
        closedir(directory);
        return 0;
    }
    if(errno == ENOTDIR)
    {
        return 1;
    }
    return -1;
}
////====================================================================================================================
void url_to_folders(char *pathMkdir, char* server, char* path,char* str) {
    char * writePtr = pathMkdir;
    char* pos;
    if( access( server, F_OK ) != 0 ) {
        mkdir(server, 0777);
    }
    strcpy(writePtr, server);
    writePtr+= strlen( server);
    strcpy(writePtr, "/");
    writePtr++;
    if(path[0] == '/') {
        path++;
    }
    while ((pos = strchr(path, '/'))){
        strncat(writePtr, path, (strlen(path)- strlen(pos)));
        writePtr += (strlen(path)-strlen(pos));
        strcpy(writePtr, "/");
        path += (strlen(path)- strlen(pos))+1;
        mkdir(pathMkdir, 0777);
        writePtr++;
    }

    if(strlen(path)){
        strcpy(writePtr, path);
    }
    else{
        strcpy(writePtr, "index.html");
    }
}
////=====================================================================================================================
int check_file(char* request,int serving_socket, char* host,char* path, char * httpPro){

    printf("HTTP request =\n%s\nLEN = %ld\n", request, strlen(request));
    FILE* flist = NULL;
    char* pathMkdir;
    if(strcmp(path,"/")==0){ // path = '/'
        strcpy(path, "/index.html");
    }
//    printf("111111111111111\n");
    char *ipAdd = malloc(sizeof(char)*(strlen(host) + strlen(path))+1);
    strcpy(ipAdd, host);
    strcat(ipAdd,path);//host+path
    char rbuf[BUFLEN];
    int flag = 0;// not have 200 ok
    int flag1 = isFile(ipAdd);
    if(access(ipAdd,F_OK) == 0 && flag1 == 1 ) {// file exist
        flist = fopen(ipAdd, "r");
        if(flist == NULL){
            error_num(500,serving_socket);//500
            return 1;
        }
        int contentLength = 0;
        fseek(flist, 0L,SEEK_END);
        contentLength = (int)ftell(flist);
        fseek(flist, 0L,SEEK_SET);
//        printf("2222222222222222222\n");

        char* heders;
        char* tempType = get_mime_type(ipAdd);
        if(tempType == NULL){
            heders = malloc(sizeof(char)*(35 + sizeof(contentLength)+1));
            heders[35 + sizeof(contentLength)] = '\0';
            sprintf(heders,"Content-length: %d\nconnection: Close\n",contentLength);
        }
        else{//tempType != NULL
            heders = malloc(sizeof(char)*(50 + strlen(tempType)+sizeof(contentLength)+1));
                   heders[50 + strlen(tempType)+sizeof(contentLength)] = '\0';
            sprintf(heders,"Content-Type: %s\nContent-Length: %d\nConnection: close\n", tempType, contentLength);
        }
        write(serving_socket,heders, strlen(heders));
        while(fgets(rbuf, 999, flist) > 0){//print the file
            write(serving_socket, rbuf, strlen(rbuf));
        }
        free(ipAdd);
        printf("File is given from local filesystem\n");
//        close(serving_socket);
        fclose(flist);
        free(heders);
    }
    else {//file not eccess
        int total = 0;
        struct sockaddr_in serv_addr;
        struct hostent *server;
        int sockfd;
//        printf("HTTP request =\n%s\nLEN = %ld\n", httpSock, strlen(httpSock));
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            perror("socket");
        server = gethostbyname(host);
        if (server == NULL)
        {
            error_num(404,serving_socket);//404
            return 1;
        }
        serv_addr.sin_family = AF_INET;
        bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(80);
        ssize_t rc = connect(sockfd, (const struct sockaddr *) &serv_addr, sizeof(serv_addr));
        if (rc < 0)
            perror("connect failed:");
        // send and then receive messages from the server

        char *request1 = malloc( sizeof (char)* (26+ strlen(path)+ strlen(host)+1));
        sprintf(request1, "GET %s %s\r\nhost: %s\r\n\r\n", path, httpPro, host);
        printf("%s\n", request1);
        write(sockfd, request1, strlen(request1));

        rc = read(sockfd, rbuf, 15);
        if(rc< 0){
            perror("read");
        }
        rbuf[rc] = '\0';
        total += (int)strlen(rbuf);
        write(serving_socket, rbuf, strlen(rbuf));
        if(strcmp(rbuf,"HTTP/1.0 200 OK") == 0|| strcmp(rbuf,"HTTP/1.1 200 OK") == 0) {//check if 200 ok is the enswer
            flag = 1;
            pathMkdir = malloc(strlen(ipAdd)+1*sizeof(char));
            if(pathMkdir == NULL){
                perror("malloc");
            }
            url_to_folders( pathMkdir, host, path,ipAdd);
            flist = fopen(pathMkdir, "w+");
            free(pathMkdir);
        }
        const char *pattern = "\r\n\r\n";
        const char *patp = pattern;
        while ((rc = recv(sockfd, rbuf, BUFLEN - 1, 0)) > 0) {
            if(rc<0){
                //PEROR OR 501?
            }
            rbuf[rc]='\0';
            total += (int)strlen(rbuf);
            write(serving_socket, rbuf, strlen(rbuf));
            if (flag == 1) {
                for (int i = 0; i < rc; i++) {
                    if (*patp == 0) {
                        fwrite(rbuf + i, 1, rc - i, flist);
                        break;
                    } else if (rbuf[i] == *patp) ++patp;
                    else patp = pattern;
                }
            }
        }
        free(request1);
        close(serving_socket);
//        free(ipAdd);// rase num of bayts
        close(sockfd);
        if(flag == 1)
            fclose(flist);
        printf("File is given from origin server\n");
        printf("\n Total response bytes: %d\n", total);
    }
    return 0;
//
}
////--------------------------------------------------------------------------------------------------------------------
unsigned int ip_to_int(const char * ip){
    unsigned v = 0;
    int i;
    const char * start;
    start = ip;
    for (i = 0; i < 4; i++)
    {
        char c;
        int n = 0;
        while (1)
        {
            c = * start;
            start++;
            if (c >= '0' && c <= '9')
            {
                n *= 10;
                n += c - '0';
            }
            else if ((i < 3 && c == '.') || i == 3)
                break;
            else
                return INVALID;
        }
        if (n >= 256)
            return INVALID;
        v *= 256;
        v += n;
    }
    return v;
}
////--------------------------------------------------------------------------------------------------------------------
char** read_lines(FILE* txt, int* count) {
    array = NULL; /* Array of lines */
    int i; /* Loop counter */
    char line[100]; /* Buffer to read each line */
    int line_length; /* Length of a single line */
/* Get the count of lines in the file */
    line_count = 0;
    while (fgets(line, sizeof(line), txt) != NULL) {
        line_count++;
    }
    /* Move to the beginning of file. */
    rewind(txt);
    /* Allocate an array of pointers to strings
     * (one item per line). */
    array = malloc(sizeof(char *)*line_count+1 );
    if (array == NULL) {
        return NULL; /* Error */
    }
    /* Read each line from file and deep-copy in the array. */
    for (i = 0; i < line_count; i++) {
        /* Read the current line. */
        fgets(line, sizeof(line), txt);
        /* Remove the ending '\n' from the read line. */
        line_length = (int)strlen(line);
        line[line_length] = '\0';
        line_length--; /* update line length */
        /* Allocate space to store a copy of the line. +1 for NUL terminator */
        array[i] = malloc(line_length +1);
        /* Copy the line into the newly allocated space. */
        strcpy(array[i], line);
        array[i][line_length-1] = '\0';
    }
    /* Write output param */
    *count = line_count;
    /* Return the array of lines */
    return array;
}
////--------------------------------------------------------------------------------------------------------------------
int checkFilter(char* host, int serving_socket){
    char *ip;
//    ip = malloc(sizeof (char)*100);
    uint32_t ip1,ip2;
    int num;
    struct hostent *host_entry;
    host_entry  = gethostbyname(host);
    if(host_entry == NULL){
//        printf("wwwwwwwwwwwwwwwwww\n");
        return 1;
    }
    ip = inet_ntoa(*((struct in_addr*)
            host_entry->h_addr_list[0]));
    ip2 = ip_to_int(ip);
    for(int i = 0; i < line_count; i++){
        if(!isdigit(array[i][0])){//www...
            if(strcmp(host,array[i])==0){
                error_num(403, serving_socket);//403
//                printf("qqqqqqqqqqqqqqqq\n");
                return 1;
            }
            host_entry  = gethostbyname(array[i]);
            if(host_entry == NULL){
//                printf("eeeeeeeeeeeeeeeee\n");
                return 1;
            }
            ip = inet_ntoa(*((struct in_addr*)
                    host_entry->h_addr_list[0]));
            ip1 = ip_to_int(ip);
        }
        else{//112.345.789
            char *cpy = malloc(sizeof(char)*strlen(array[i]));
            strcpy(cpy,array[i]);
            ip1 =ip_to_int(array[i]);
            char *token = strtok(cpy, "/");
            token=strtok(NULL, " \n");
            if(token != NULL){
                num = atoi(token);
            }
            free(cpy);//rase num of bytes
        }
        int cidr = num;
        u_int32_t ipaddr_from_pkt = ip1;     // pkt coming from 104.40.141.105
        u_int32_t ipaddr_from_list = ip2;    // 104.40.0.0
        int mask = (-1) << (32 - cidr);

        if ((ipaddr_from_pkt & mask) == (ipaddr_from_list)) {
            error_num(403,serving_socket);//403
//            printf("ttttttttttttttttttt\n");
            return 1;
        }
    }
    return  0;
}
////--------------------------------------------------------------------------------------------------------------------
int sendToConvert(char* host,int serving_socket){
    uint32_t ip2;
    FILE *file = fopen(pathFilter,"r");
    if(file == NULL)
    {
        printf("error: fopen\n");
        exit(1);
    }
    array = NULL;
    /* Read lines from file. */
    array = read_lines(file, &line_count);
    int j = checkFilter(host,serving_socket);
    if(j == 1){
        return 1;
    }
    fclose(file);
    return 0;
}
////--------------------------------------------------------------------------------------------------------------------
int errors(char* str1, int serving_socket){
    char* str3 = malloc(sizeof(char*)* strlen(str1)+1);
    str3 = strcpy(str3, str1);
    char* strcopy = malloc(sizeof(char*)* strlen(str1)+1);
    strcopy = strcpy(strcopy, str1);
    char* str2 = malloc(sizeof (char*)* strlen(str1)+1);
    strcpy(str2,str1);
    int profit = 0;// specie between words
    char *flag = NULL;
    char *host = NULL;
    str3 = strtok( str3, "\r\n");//first sentence
    for(int i = 0; i < strlen(str3); i++){//num of word in str1
        if(str3[i] == ' ') {
            profit++;
        }
    }
    char* path;
    char* httpPro;
    strcpy(str3, strcopy);
    str3 = strtok(str3, " ");
    if(profit == 2){//3 word in str
        path =  strtok(NULL, " ");
        httpPro =  strtok(NULL, "\r\n");
        if(strcmp(httpPro, "HTTP/1.0") != 0 && strcmp(httpPro, "HTTP/1.1") != 0){//dont have 1.0 or 0.0
            error_num(400,serving_socket);//400
            return 1;
            //free strcopy, str2, str3
        }

        str2 = strtok(str2,"\r\n");
        while((str2 = strtok(NULL, " ")) != NULL){
            if ((flag = strstr(str2, "host:"))) {//have host
                break;
            }
            if ((flag = strstr(str2, "Host:"))) {//have Host
                break;
            }
        }
        if(flag == NULL){// dont have host or Host
            error_num(400,serving_socket);//400
            return 1;
        }
        else{
            host = strtok(NULL, " \r\n\r\n");
        }

        ////
        if(strncmp(path, "http://", strlen("http://")) == 0){// have after GET http://
            int number = (int)(7+strlen(host));
            for(int i = 0; i < number; i++){
                path++;
            }
        }
        ////
        if(strcmp(str3, "GET") != 0 ){//don't have GET
            error_num(501,serving_socket);//501
            return 1;
        }
    }
    else{//no host or not have 3 words in str1
        error_num(400,serving_socket);//400
        return 1;
    }
    int j = sendToConvert(host,serving_socket);
    if(j == 1){
        return 1;
    }
    j = check_file(str1, serving_socket, host, path, httpPro);
    if(j == 1){
        return 1;
    }
    free(str3);
//    free(str2);//falled
    free(strcopy);
 return 0;
}
////--------------------------------------------------------------------------------------------------------------------
int handle_client(void* serving_socket){
    int serving_socket2 = *((int*)serving_socket);
    size_t rc;
    char rbuf[BUFLEN] ;//= "GET /t/ex2 HTTP/1.0\r\nhost: www.ptsv2.com\r\n\r\n";//GET /t/ex2 HTTP/1.0\r\nhost: www.ptsv2.com
    rc = read(serving_socket2, rbuf, BUFLEN-1);
    rbuf[rc] = '\0';
    if(rc<0) //read failed
    {
        close(serving_socket2);
        error_num(500,serving_socket2);
    }
    int check = errors(rbuf, serving_socket2);
    if(check == 1){//404/500/501/404....
        close(serving_socket2);
        return 1;
    }
//    else{
//        check_file(rbuf);
//    }

////
//    rbuf="GET /t/ex2 HTTP/1.0\r\nhost: www.ptsv2.com\r\n\r\n";
//    int check = errors(rbuf, serving_socket2);
//    if(check == 1){//404/500/501/404....
//        close(serving_socket2);
//        return 1;
//    }
////

    close(serving_socket2);
    return 0;
}
////--------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    //check the number of arguments from the shell
    if (argc != 5){
        printf("Usage: proxyServer <port> <pool-size> <max-requests-number> <filter>\n");
        exit(EXIT_FAILURE);
    }
    int main_socket, serving_socket;
    pathFilter = argv[4];
    int pool_size = atoi(argv[2]);
    if(pool_size < 0){
        printf("Usage: proxyServer <port> <pool-size> <max-requests-number> <filter>\n");
        exit(EXIT_FAILURE);
    }
    int max_requests_number = atoi(argv[3]);

    if(max_requests_number < 0){
        printf("Usage: proxyServer <port> <pool-size> <max-requests-number> <filter>\n");
        exit(EXIT_FAILURE);
    }

    if(atoi(argv[1]) < 0){//port<0
        printf("Usage: proxyServer <port> <pool-size> <max-requests-number> <filter>\n");
        exit(EXIT_FAILURE);
    }
    threadpool *pool = create_threadpool(pool_size);
    struct sockaddr_in serv_addr;
    int rc;
    main_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (main_socket < 0)
        perror("socket failed");
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));
    rc = bind(main_socket, (struct sockaddr *) &serv_addr,  sizeof(serv_addr));
    if (rc < 0)
        perror("bind failed");

    rc = listen(main_socket, 5);
    if (rc < 0)
        perror("listen failed");

   for(int i = 0; i < max_requests_number; i++) {
        serving_socket = accept(main_socket, NULL, NULL);
        if (serving_socket < 0)
            perror("accept failed");
        dispatch(pool,  handle_client, &serving_socket);
    }
    close(main_socket);
    destroy_threadpool(pool);

    for(int i = 0; i < line_count; i++){
        free(array[i]);
    }
    free(array);
    close(main_socket);
    close(serving_socket);
    return 0;
}
