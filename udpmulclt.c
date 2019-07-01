/***
 File Name: udpmulclt.c
 Author: alen6516
 Created Time: 2019-06-27
***/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>


#define _INT_NAME (64)
#define _INT_TEXT (512)

#define CERR(fmt, ...) \
    fprintf(stderr, "[%s:%s:%d][error %d:%s]" fmt "\r\n", \
    __FILE__, __func__, __LINE__, errno, strerror(errno), ##__VA_ARGS__)


#define CERR_EXIT(fmt, ...) \
    CERR(fmt, ##__VA_ARGS__), exit(EXIT_FAILURE)


#define IF_CHECK(code) \
    if ((code) < 0) \
        CERR_EXIT(#code)


int _exit(int, pid_t);


typedef struct {
    char type;
    char name[_INT_NAME];
    char text[_INT_TEXT];
} umsg;


int main(int argc, char *argv[]) {

    int sd, rt;
    struct sockaddr_in addr = { AF_INET };
    socklen_t alen = sizeof(addr);
    pid_t pid;
    umsg msg = { '1' };
    

    // check if arg legal
    if (argc != 4) {
        fprintf(stderr, "uage %s [ip] [port] [name]\n", argv[0]);
        exit(EXIT_FAILURE);
    }


    // check if port legal
    if ((rt = atoi(argv[2])) < 1024 || rt > 65535) {
        CERR("atoi port = %s is error!", argv[2]);
    }


    // check if ip legal
    IF_CHECK (inet_aton(argv[1], &(addr.sin_addr)));
    addr.sin_port = htons(rt);


    // copy client's name
    strncpy (msg.name, argv[3], sizeof(_INT_NAME) -1);


    // create socket connection
    IF_CHECK (sd = socket(AF_INET, SOCK_DGRAM, 0));


    // send login msg to server
    IF_CHECK (sendto (sd, &msg, sizeof(msg), 0, (struct sockaddr *) &addr, alen));


    // create a child process, child process sending, fathor process receiveing
    IF_CHECK (pid = fork());
    if (pid == 0) {
        
        // ignore exit to prevent becoming zombi process
        signal(SIGCHLD, SIG_IGN);
        
        
        // when quit
        while (fgets(msg.text, _INT_TEXT, stdin)) {
            if (strcasecmp(msg.text, "quit\n") == 0) {
                msg.type = '3';

                
                // send msg and check
                IF_CHECK (sendto (sd, &msg, sizeof(msg), 0, (struct sockaddr *) &addr, alen));
                break;
            }


            // or send normal msg
            msg.type = '2';
            IF_CHECK (sendto (sd, &msg, sizeof(msg), 0, (struct sockaddr *) &addr, alen));
        }


        // close and kill fathor process
        close(sd);
        kill(getppid(), SIGKILL);
        exit(0);
    }


    // fathor process receiving data
    for (;;) {
        bzero (&msg, sizeof(msg));
        IF_CHECK (recvfrom (sd, &msg, sizeof(msg), 0, (struct sockaddr*) &addr, &alen));
        msg.name[_INT_NAME-1] = msg.text[_INT_TEXT-1] = '\0';
        switch (msg.type) {
            case '1':
                printf("%s 登入了聊天室\n", msg.name);
                break;
            case '2':
                printf("%s 說了: %s\n", msg.name, msg.text);
                break;
            case '3':
                printf("%s 退出了聊天室\n", msg.name);
                break;
            default:
                fprintf(stderr, "msg is error! [%s:%d] => [%c:%s:%s]\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), msg.type, msg.name, msg.text);
                _exit(sd, pid);
        }
    }
}


int _exit(int sd, pid_t pid) {
    close(sd);
    kill(pid, SIGKILL);
    waitpid(pid, NULL, -1);
    return 0;
}
