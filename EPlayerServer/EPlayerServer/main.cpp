#include "Process.h"
#include "Logger.h"
#include "ThreadPool.h"

//Log子进程
int CreateLogServer(CProcess* ProcLog)
{
	Logger LogServer;
	int ret = LogServer.Start();
	if (ret != 0)
	{
		printf("%s(%d):<%s> ret=%d,errno=%d,msg=%s\r\n", __FILE__, __LINE__, __FUNCTION__, ret, errno, strerror(errno));
		return -1;
	}
	int fd = 1;
	while (true)
	{
		ret = ProcLog->RecvFD(fd);
		if (fd <= 0)
		{
			printf("%s(%d):<%s> ret=%d,errno=%d,msg=%s\r\n", __FILE__, __LINE__, __FUNCTION__, ret, errno, strerror(errno));
			break;
		}
	}
	return 0;
}

//主进程调用
int LogTest()
{
	printf("%s(%d):<%s> pid=%d tid=%d\r\n", __FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self());
	char buf[] = "Nice to meet you 天命人";
	usleep(1000 * 100);//等待log进程启动
	TRACEI("Hello World! %d %s %c %f %g %e", 1, buf, 'A', 2.4f, 2.1234l, 1.2345);
	LOGD << "I love you" << buf << "哈哈" << 1 << 'B' << 3.14f << 0.2232324;
	DUMPE((void*)buf, (size_t)sizeof(buf));
	return 0;
}

int CreateClientServer(CProcess* ProcClient)
{
	int fd = -1;
	usleep(10 * 1000);
	ProcClient->RecvFD(fd);
	lseek(fd, 0, SEEK_SET);
	char buffer[10] = "";
	read(fd, buffer, sizeof(buffer));
	printf("%s(%d):<%s> buffer=%s\r\n", __FILE__, __LINE__, __FUNCTION__, buffer);
	return 0;
}

int main()
{
	printf("==================================\r\n");
	//CProcess::CreateDaemon();
	CProcess logServer;
	//CProcess clientServer;
	int ret = logServer.SetEntryFunc(CreateLogServer, &logServer);
	ret = logServer.CreateSubProcess();
	if (ret != 0)return -1;

	LogTest();
	Thread log(LogTest);
	ret = log.Start();
	if (ret != 0)return -2;
	ThreadPool threads;
	ret = threads.Start(4);
	if (ret != 0)return -3;
	threads.AddTask(LogTest);
	threads.AddTask(LogTest);
	threads.AddTask(LogTest);
	threads.AddTask(LogTest);

	(void)getchar();
	//由于-1文件描述符是不可访问的，所以发送这个文件描述符会报bad file descriptor
	ret = logServer.SendFD(-1);
	if (ret != 0)
	{
		printf("%s(%d):<%s> ret=%d errno=%d msg=%s\r\n", __FILE__, __LINE__, __FUNCTION__, ret, errno, strerror(errno));
		return -3;
	}
	(void)getchar();
	/*ret = clientServer.SetEntryFunc(CreateClientServer, &clientServer);
	if (ret == -1)
	{
		printf("%s(%d):<%s> pid=%d\r\n", __FILE__, __LINE__, __FUNCTION__, getpid());
		return -1;
	}
	clientServer.CreateSubProcess();
	int fd = open("./test.txt", O_RDWR | O_CREAT | O_APPEND);
	ret = clientServer.SendFD(fd);
	char buffer[10] = "Hello";
	write(fd, buffer, sizeof(buffer));
	close(fd);*/
	return 0;
}