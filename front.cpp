#include<iostream>
#include<string>
#include<vector>
#include<stdio.h>
#include<sys/types.h>
#include<stdlib.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<sys/wait.h>
#include<sys/prctl.h>
#include<sys/stat.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<signal.h>
#include<errno.h>

#define SIZE 1024

using namespace std;

const char* file_name = "/home/cyril-ki/Cyril_KI/pipe/command_res";
const char* syn_pipe_name = "/home/cyril-ki/Cyril_KI/pipe/syn_file";

typedef struct msg {
    long int type;
    char text[SIZE];
}MSG;

//create msgQueue
int msgQueue() {
    key_t key;
    int queId;
    key = ftok("/etc/profile", 11);
    if(key == -1) {
        perror("ftok");
        exit(-1);
    }
    queId = msgget(key, IPC_CREAT | 0666);
    if(queId == -1) {
        perror("msgget");
        exit(-1);
    }
    return queId;
}

int sendMessage(MSG m, int id) {
    MSG msg;
    memset(&msg, 0x00, sizeof(MSG));
    msg.type = m.type;
    strcpy(msg.text, m.text);
    if(msgsnd(id, (const void *)&msg, strlen(msg.text), IPC_NOWAIT) < 0) {
        perror("msgsnd");
     exit(-1);
    }
    return 0;
}

int createPipe() {
    if(mkfifo(file_name, 0644) < 0) {
        if(errno != EEXIST) {
            perror("mkfifo");
            exit(-1);
        }
    }
    return 0;
}

int synCreatePipe() {
    if(mkfifo(syn_pipe_name, 0644) < 0) {
        if(errno != EEXIST) {
            perror("mkfifo");
            exit(-1);
        }
    }
    return 0;
}

void readPipe() {
    int fd;
    if((fd = open(file_name, O_RDONLY)) < 0) {
        perror("open");
        exit(-1);
    }
    umask(0);
    createPipe();
    synCreatePipe();
    printf("\033[37m");
    for(int i = 0; i < 25; i++) {
        cout << "-";
    }
    cout << endl;
    int cnt = 0;
    while(1) {
        char buf[100];
        int ret = read(fd, buf, 100);
        if(ret < 0) {
            perror("read");
            exit(-1);
        }
        if(ret == 0) {   //empty
            break;
        }
        printf("\033[33m");
        cout << buf;
    }
    printf("\033[37m");
    for(int i = 0; i < 25; i++) {
        cout << "-";
    }
    cout << endl;
    printf("\033[36m");
    cout << "\nExecuted successfully!\n\n";
    close(fd);
}

string getCurrentwd() {
    char pwd[255];
    if(!getcwd(pwd, 255)) {
        perror("getcwd");
        exit(-1);
    }
    string str = pwd;
    return str;
}

void convertCommand(pid_t pid, string str, MSG* msg) {
    char buf[SIZE];
    memset(buf, 0, sizeof(buf));
    if(str == "dir") {
        memcpy(msg->text, "ls\0", 3);
    } else if(str.substr(0, 4) == "move") {
        //replace
        str.replace(0, 4, "mv");
        str.copy(buf, str.size(), 0);
        *(buf + str.size()) = '\0';
        memcpy(msg->text, buf, sizeof(buf));
    } else if(str.substr(0, 6) == "rename") {
        //replace
        str.replace(0, 6, "mv");
        str.copy(buf, str.size(), 0);
        *(buf + str.size()) = '\0';
        memcpy(msg->text, buf, sizeof(buf));
    } else if(str.substr(0, 3) == "del") {
        //replace
        str.replace(0, 3, "rm");
        str.copy(buf, str.size(), 0);
        *(buf + str.size()) = '\0';
        memcpy(msg->text, buf, sizeof(buf));
    } else if(str.substr(0, 2) == "cd") {
        str.copy(buf, str.size(), 0);
        *(buf + str.size()) = '\0';
        memcpy(msg->text, buf, sizeof(buf));
        //check path
        string cdStr = "";
        for(int i = 0; i < str.size(); i++) {
            if(str[i] != ' ') {
                cdStr += str[i];
            }
        }
        cdStr.replace(1, 1, "d ");
        //chdir
        string s;
        if(cdStr[3] == '/') {
            s = cdStr.substr(3, cdStr.size() - 3);
        } else {
            s = getCurrentwd() + "/" + str.substr(3, str.size() - 3);
        }
        if(chdir(s.data()) == -1) {
            perror("chdir");
            exit(-1);
        }
        string t = "cd " + s;
        memcpy(msg->text, t.data(), sizeof(buf));
    } else if(str == "cls") {
        str = "clear";
        str.copy(buf, str.size(), 0);
        *(buf + str.size()) = '\0';
        memcpy(msg->text, buf, sizeof(buf));
    } else if(str.substr(0, 4) == "type") {
        str.replace(0, 4, "cat");
        str.copy(buf, str.size(), 0);
        *(buf + str.size()) = '\0';
        memcpy(msg->text, buf, sizeof(buf));
    } else if(str.substr(0, 2) == "md") {
        str.replace(0, 2, "mkdir");
        str.copy(buf, str.size(), 0);
        *(buf + str.size()) = '\0';
        memcpy(msg->text, buf, sizeof(buf));
    } else if(str.substr(0, 2) == "rd") {
        str.replace(0, 2, "rmdir");
        str.copy(buf, str.size(), 0);
        *(buf + str.size()) = '\0';
        memcpy(msg->text, buf, sizeof(buf));
    } else if(str == "path") {
        str = "echo $PATH";
        str.copy(buf, str.size(), 0);
        *(buf + str.size()) = '\0';
        memcpy(msg->text, buf, sizeof(buf));
    } else if(str.substr(0, 4) == "copy") {
        str.replace(0, 4, "cp");
        str.copy(buf, str.size(), 0);
        *(buf + str.size()) = '\0';
        memcpy(msg->text, buf, sizeof(buf));
    } else if(str == "time") {
        str = "date";
        str.copy(buf, str.size(), 0);
        *(buf + str.size()) = '\0';
        memcpy(msg->text, buf, sizeof(buf));
    } else {
        str.copy(buf, str.size(), 0);
        *(buf + str.size()) = '\0';
        memcpy(msg->text, buf, sizeof(buf));
    }
}

void print() {
    printf("\033[33m");
    cout << "please input command:";
    printf("\033[37m");
    cout << "(";
    //green
    printf("\033[32m");
    cout << "cyril-ki@ubuntu:";
    string pwd = getCurrentwd();
    pwd.replace(0, 14, "~");
    //blue
    printf("\033[34m");
    cout << pwd.data();
    //white
    printf("\033[37m");
    cout << ")\n";
    printf("\033[33m");
}

void writePipe() {
    int fd;
    if((fd = open(syn_pipe_name, O_WRONLY | O_TRUNC)) < 0) {  //clear
        perror("open");
        exit(-1);
    }
    close(fd);
}

int main(int argc, char *argv[], char *env[]) {
    pid_t pid;
    int queId;
    char id[10];
    MSG msg;
    signal(SIGCHLD, SIG_IGN);
    //create pipe
    createPipe();
    synCreatePipe();
    //create msgQueue
    queId = msgQueue();
    if(queId == -1) {
        return 0;
    }
    sprintf(id, "%d", queId);
    pid = fork();
    if(pid < 0) {
        perror("fork");
        exit(-1);
    }else if(pid == 0){
        //start child
        prctl(PR_SET_PDEATHSIG, SIGKILL);
        if(execl("./back", "back", id, NULL) == -1) {
            perror("execl");
            exit(-1);
        }else {
            printf("back is started..\n");
        }
    }else {
        while(1) {
            string str;
            msg.type = 1;
            print();
            getline(cin, str);
            convertCommand(pid, str, &msg);
            writePipe();
            sendMessage(msg, queId);
            if(str == "exit") {
                waitpid(pid, NULL, 0);
                unlink(file_name);
                unlink(syn_pipe_name);
                exit(0);
            }
            readPipe();
        }
    }
    return 0;
}