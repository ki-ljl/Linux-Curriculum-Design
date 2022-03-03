![](https://img.shields.io/badge/Linux-Process-yellow)
# 一、内容介绍
## 1.目的与要求
学习UNIX/LINUX系统下的多进程创建、控制和通信。
## 2.主要内容
（1）Linux上的bash和Windows中的命令行有很大的不同。但是两者都有完成相似任务的命令，比如Linux上bash的ls命令的功能，类似于Windows命令行中的dir命令的功能。用C语言写一个简单的Linux终端软件，接收用户发出的类似于Windows命令行中的命令，转换成对应的Linux命令加以执行，并将执行的结果回显给用户。比如，用户输入“dir”，程序实际返回“ls”的内容。

（2）软件包含前、后台两个程序，用户启动前台程序时，前台程序自行启动后台程序。前台程序提供界面，负责接收用户输入，对输入进行转换，并向后台程序发出实际要执行的指令，后台负责执行实际的指令。

## 3.设计成果要求
（1）前台程序通过fork和execl系统调用启动后台程序。

（2）前台程序创建消息队列和命名管道，通过消息队列向后台程序发送经过转换的用户命令；通过命名管道从后台程序获取命令执行的结果，并显示在终端。后台程序可以通过popen来执行转换后的命令。

（3）至少实现如下Windows——Linux对照命令：dir——ls，rename——mv，move——mv，del——rm，cd——cd（pwd），exit——exit。

（4）当用户输入exit时，前台程序指示后台程序结束，在后台程序结束后，前台程序退出；在此之前，用户的输入都被作为一条命令进行处理。

# 二、整体功能及设计
## 1.程序流程
整个Linux终端软件由两个程序组成：前台程序front.cpp和后台程序back.cpp。

（1）前台程序首先需要创建消息队列和命名管道，然后通过fork和execl系统调用启动后台程序。随后前台程序接受用户的输入，将转换后的命令通过消息队列发送给后台程序。

（2）后台程序开始运行后尝试从消息队列中获取消息，读取成功后利用popen来执行读取到的命令，并写入命名管道。

（3）前台程序在后台程序将命令运行完成后，读取命名管道中的内容，然后继续发送下一条指令。

在整个程序运行过程中，使用一个同步管道来维护前后台程序间的同步：前台程序发送完消息后等待后台程序将执行结果写入后再读取打印，后台程序等待前台程序发送完命令后再读取并执行。

流程图如图1所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/51b583f6136d42e78b4beba4ff0936f8.png#pic_center)
## 2.同步功能
在通信过程中，必须保证前后台程序同步，否则可能会得到错误的结果。所谓同步，是指前台程序应该等待后台程序处理完一条指令后再读取结果并打印，然后再发送下一条指令。同时后台程序必须等待前台程序发送完命令后再从消息队列中读取该命令然后执行。

为了保证同步，在消息发送和接收前各加一个同步管道的读写操作。

前台程序的同步代码为：
```cpp
writeSynPipe();                      //写同步管道
sendMessage(msg, queId);            //发送消息
readResPipe();                       //读结果管道并打印  
```
后台程序的同步代码为：
```cpp
readSynPipe();                       //读同步管道
if(readMessage(queId, &m) == -1) {
      continue;                       //继续尝试
}
execCommand(m.text, resCommand);    //执行命令
writeResPipe(resCommand);            //将结果写入管道
```
其中synPipe是同步管道，resPipe是保存命令执行结果的管道。

## 3.命令转换
前台程序接收到用户输入的函数后，利用命令转换函数convertCommand()将Windows命令转换为对应的Linux命令。本次实验实现的Windows-Linux命令转换表如下所示。
|Windows命令  |Linux命令  |作用  |
|--|--|--|
|dir  |ls  |打印当前目录下的文件名  |
|rename  |mv  |重命名文件  |
|cd  |cd+pwd  |切换工作路径  |
|move  |mv  |移动文件  |
|del  |rm  |删除文件  |
|exit  |exit  |退出  |
|md  |mkdir  |创建目录  |
|rd  |rmdir  |删除目录  |
|path  |echo $PATH  |打印可执行文件路径  |
|copy  |cp  |复制文件  |
|time  |date  |显示系统时间  |
|type  |cat  |输出文件到屏幕  |
|cls  |clear  |清屏  |

其中前六条命令为本次实验要求必须实现的命令，后七条命令为额外实现的命令。

## 4.cd命令的处理
当前台程序输入cd命令后，通过消息队列发送给后台程序。后台程序通过popen来执行cd命令，由于popen函数的机理是重新fork一个子进程来处理命令，因此后台程序处理完之后并不会改变前台程序的工作路径。

为了解决这一情况，对于每一条用户输入的cd命令，获取路径后在前面加上当前路径（通过getcwd函数获取），形成绝对路径，然后在前台程序中使用chdir函数来切换工作路径。

## 5.exit命令的处理
当用户输入exit命令时：前台程序将命令发送给后台程序，然后调用waitpid函数等待后台程序退出，此时前台程序被阻塞。后台程序收到exit命令后，立即调用exit(0)退出，退出后前台程序被激活，然后删除管道文件，再调用exit(0)退出。至此，前后台程序都成功退出。

# 三、实验结果
## 1.实验环境
操作系统：Ubuntu-20.04
编程语言：C++11
IDE：Visual Studio Code 1.63.2
编译器：g++ (Ubuntu 9.3.0-17ubuntu1~20.04) 9.3.0

## 2.makefile
创建makefile文件，并输入以下内容：
![在这里插入图片描述](https://img-blog.csdnimg.cn/22342371508d47598408c167c08fe083.png#pic_center)
## 3.程序界面
打开Linux终端，输入make命令：

![在这里插入图片描述](https://img-blog.csdnimg.cn/b2f145e4e55f4f65bb04e630f124bec7.png#pic_center)

然后输入：./front，开始运行前台程序，界面如下所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/4d87d9151ce64166b407f5d69eec1cec.png#pic_center)

此时后台程序已经启动，等待用户输入。提示语后括号中的内容为当前前台程序所处的工作路径。

## 4.命令执行
### 4.1 dir-ls
输入dir命令，实际执行ls命令，结果如下图所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/ffcc169dbad2489388fe6c4934180ed9.png#pic_center)

可以发现，成功打印了当前文件夹Cyril_KI下的所有文件。

### 4.2 rename-mv
输入rename t.c f.c，实际执行mv t.c f.c命令；然后输入dir，结果如下所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/593b25e8400c48a4b64adbdeb590936a.png#pic_center)

可以看到，目录中原t.c文件已经成功更名为f.c文件。

### 4.3 cd-cd(pwd)
输入cd A/命令，实际执行cd A/+pwd命令，结果如下所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/730e5a8e125a47ddb6b1f09d716aa933.png#pic_center)

可以看到，当前工作路径已经切换到了A目录下。

### 4.4 move-mv
输入move命令前，先输入cd ..命令，切换到原始目录：
![在这里插入图片描述](https://img-blog.csdnimg.cn/ce6794c24cd54a08af811e5d83dd853a.png#pic_center)

然后输入move f.c A/，实际执行mv f.c A/命令，将f.c移动到文件夹A下；然后cd A/，最后dir，结果如下所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/540ef018724b403f875dca6ac2fad6ab.png#pic_center)

可以发现，f.c被移动到了A文件夹下。

### 4.5 del-rm
输入del a，实际执行rm a命令；然后输入dir，实际执行ls命令，结果如下所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/d5e7dcba742646e295af803e809fe9a4.png#pic_center)

可以看到，文件夹A下a文件已经被删除。

### 4.6 exit-exit
先输入ps，显示当前进程；然后输入exit命令，结果如下所示：

![在这里插入图片描述](https://img-blog.csdnimg.cn/2f02142a7e1d4d218232fe5cc17525a2.png#pic_center)

可以看到，输入exit后程序自动退出，并且此时再次输入ps命令后front和back进程已经被清除。

### 4.7 md-mkdir
重新输入./front启动程序，依次输入md T/和dir命令，结果如下所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/f3080dc86ad34a8aa8a6e1c4d42fe7b6.png#pic_center)

可以看到，当前文件夹下新增了一个T文件夹。

### 4.8 rd-rmdir
输入rd T/，然后输入dir，结果如下所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/ce9219718c324bd6b4717105ed3a491c.png#pic_center)

可以看到，当前文件夹下的T文件夹已经被删除。

### 4.9 path-echo $PATH
输入path，实际执行echo $PATH，结果如下所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/235a760302b34e9c97c70ca0f753c509.png#pic_center)

### 4.10 copy-cp
输入copy G.cpp GG.cpp，实际执行cp G.cpp GG.cpp，然后输入dir，结果如下所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/ea33fa4134984b00b69183154f1729b8.png#pic_center)

可以看到，当前文件夹下多出了一个GG.cpp文件。

### 4.11 time-date
输入time命令，实际执行date命令，结果如下所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/77acca970f7c4699b8b1685ec4a19c52.png#pic_center)

可以看到，成功打印出了系统时间。

### 4.12 type-cat
输入type makefile，实际执行cat makefile，结果如下所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/3c160e97bcd947978aae2a07627497ab.png#pic_center)

### 4.13 cls-clear
输入cls命令，实际执行clear命令，结果如下所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/18d8874be7424a70b97b26b9fc605a67.png#pic_center)

# 四、总结与改进
本次实验顺利完成了任务书中的各个要求，并且都得到了正确的结果。本次实验的亮点有：

（1）除了任务书要求完成的基本命令转换外，还额外完成了md等七条Windows命令的转换。

（2）研究了popen函数的机理，在执行cd命令时使用chdir函数来对前台和后台程序的工作路径进行切换。

（3）采用命名管道对前后台程序的通信进行同步。

（4）不同输出采用不同颜色，使结果一目了然。

不足之处与改进：

（1）Linux的终端界面比较简洁，可以考虑实现图形界面。

（2）没有考虑太多的异常处理，可以考虑针对用户错误的输入做出相应提示。
