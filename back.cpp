#include<iostream>
#include<vector>
#include<stdio.h>
#include<sys/types.h>
#include<stdlib.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>

#define SIZE 1024

using namespace std;

const char* file_name = "/home/cyril-ki/Cyril_KI/pipe/command_res";
const char* syn_pipe_name = "/home/cyril-ki/Cyril_KI/pipe/syn_file";

typedef struct msg {
    long int type;
    char text[SIZE];
}MSG;

int createPipe() {
    if(mkfifo(file_name, 0644) < 0) {
        if(errno != EEXIST) {
            perror("mkfifo");
            exit(-1);
        }
    }
    return 0;
}

string getCurrentwd() {
    char pwd[255];
    if(!getcwd(pwd, 255)) {
        perror("getcwd");
        exit(-1);
    }
    string str = pwd;
    return str;
}

void readPipe() {
    int fd;
    if((fd = open(syn_pipe_name, O_RDONLY)) < 0) {
        perror("open");
        exit(-1);
    }
    close(fd);
}

void writePipe(vector<string> res) {
    int fd;
    if((fd = open(file_name, O_WRONLY | O_TRUNC)) < 0) {  //clear
        perror("open");
        exit(-1);
    }
    umask(0);
    createPipe();
    //write
    for(string str : res) {
        char buf[100];
        str.copy(buf,str.size(),0);
        *(buf+str.size()) = '\n';
        *(buf+str.size() + 1) = '\0';
        write(fd, buf, sizeof(buf));
    }
    close(fd);
}

int readMessage(int id, MSG *m) {
    int res;
    MSG msg;
    memset(&msg, 0x00, sizeof(MSG));
    if((res = msgrcv(id, (void *)&msg, sizeof(msg.text), 1, 0)) < 0) {
        perror("msgrcv");
		return -1;
    }
    m->type = msg.type;
    strcpy(m->text, msg.text);
    return res;
}

void execCommand(const char *cmd, vector<string> &res) {
    res.clear();
    FILE *pp = popen(cmd, "r");
    string str = getCurrentwd();
    if (!pp) {
        perror("popen");
        exit(-1);
    }
    char temp[1024];
    while (fgets(temp, sizeof(temp), pp) != NULL) {
        if (temp[strlen(temp) - 1] == '\n') {
            temp[strlen(temp) - 1] = '\0';
        }
        res.push_back(temp);
    }
    pclose(pp);
    string t = cmd;
    if(t.substr(0, 2) == "cd") {
        string dir = t.substr(3, t.size() - 3 - 4);
        if(chdir(dir.data()) == -1) {
            perror("chdir");
            exit(-1);
        }
    }
}

int main(int argc, char *argv[], char *env[]) {
    int res;
    MSG m;
    int queId = atoi(argv[1]);
    while(1) {
        readPipe();
        memset(&m, 0x00, sizeof(MSG));
        res = readMessage(queId, &m);
        if(res < 0) {
            continue;
        }
        vector<string> resCommand;
        string str = m.text;
        if(str == "exit") {
            exit(0);
        }
        if(str.substr(0, 2) == "cd") {
            char buf[5] = ";pwd";
            strcat(m.text, buf);
        }
        cout << endl;
        printf("\033[36m");
        printf("the result is:\n");
        execCommand(m.text, resCommand);
        writePipe(resCommand);
    }
    return 0;
}
