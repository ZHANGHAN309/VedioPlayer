#include <cstdio>

#include <unistd.h>//包含一些数据类型和api函数
#include <functional>//与模板有关
#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

class FuncBase
{
public:
	FuncBase() {}
	virtual ~FuncBase() {}//使用虚析构保证子类可以被析构
	virtual int operator()() = 0;
};

template<typename _FUNC_, typename... _ARG_>
class CFunction :public FuncBase
{
public:
	CFunction(_FUNC_ func, _ARG_... args)
		:m_binder(std::forward<_FUNC_>(func), std::forward<_ARG_>(args)...)//std::forward表示原样转发
	{
	}
	virtual ~CFunction() {}
	virtual int operator()()
	{
		return m_binder();
	}
private:
	typename std::_Bindres_helper<int, _FUNC_, _ARG_...>::type m_binder;
};

class CProcess
{
public:
	CProcess()
	{
		m_func = NULL;
		memset(pipes, 0, sizeof(pipes));
	}
	~CProcess()
	{
		if (m_func != NULL)
		{
			delete(m_func);
			m_func = NULL;
		}
	}
	template<typename _FUNC_, typename... _ARG_>
	int SetEntryFunc(_FUNC_ funcName, _ARG_... args)
	{
		if (m_func != NULL)return -1;
		m_func = new CFunction<_FUNC_, _ARG_...>(funcName, args...);
	}
	int CreateSubProcess()
	{
		printf("%s(%d):<%s> pid=%d\r\n", __FILE__, __LINE__, __FUNCTION__, getpid());
		if (m_func == NULL)return -1;
		int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, pipes);//创建一对套接字用于进程间通信，但是不在内核中进行，提高效率
		if (ret != 0)return -2;
		pid_t pid = fork();
		if (pid == -1)return -3;
		if (pid == 0)
		{
			printf("%s(%d):<%s> pid=%d\r\n", __FILE__, __LINE__, __FUNCTION__, getpid());
			close(pipes[0]);//子进程用第2个套接字
			pipes[0] = 0;
			(*m_func)();//子进程
			exit(0);
		}
		printf("%s(%d):<%s> pid=%d\r\n", __FILE__, __LINE__, __FUNCTION__, getpid());
		close(pipes[1]);//主进程用第1个套接字
		pipes[1] = 0;
		m_pid = pid;//主进程
		return 0;
	}

	int SendFD(int fd)
	{
		iovec iov;
		iov.iov_base = (char*)"Hello World!";
		iov.iov_len = 13;
		msghdr msg;
		bzero(&msg, sizeof(msg));
		msg.msg_iov = &iov;//设置缓冲区
		msg.msg_iovlen = 1;
		cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));
		if (cmsg == NULL)
		{
			perror("Create cmsg failed!\r\n");
			return -1;
		}
		cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		*(int*)CMSG_DATA(cmsg) = fd;
		msg.msg_control = cmsg;
		msg.msg_controllen = CMSG_LEN(sizeof(int));
		int ret = sendmsg(pipes[0], &msg, 0);
		printf("%s(%d):<%s> cmsg:%08X\r\n", __FILE__, __LINE__, __FUNCTION__, cmsg);
		free(cmsg);
		if (ret <= 0)
		{
			perror("Sendmsg failed!\r\n");
			return -2;
		}
		return 0;
	}

	int RecvFD(int& fd)
	{
		iovec iov;
		char buffer[13] = "";
		iov.iov_base = buffer;
		iov.iov_len = 13;
		msghdr msg;
		bzero(&msg, sizeof(msg));
		msg.msg_iov = &iov;//设置缓冲区
		msg.msg_iovlen = 1;
		char control_data[CMSG_SPACE(sizeof(int))] = { 0 };
		msg.msg_control = control_data;//设置控制信息
		msg.msg_controllen = sizeof(control_data);
		int ret = recvmsg(pipes[1], &msg, 0);//接收信息
		if (ret <= 0)
		{
			perror("Recvmsg failed!\r\n");
			return -2;
		}
		cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);//获取控制信息
		printf("%s(%d):<%s> cmsg:%08X\r\n", __FILE__, __LINE__, __FUNCTION__, cmsg);
		//free(cmsg);
		if (cmsg == NULL)
		{
			perror("Find first cmsghdr failed!\r\n");
			return -1;
		}
		memcpy(&fd, (int*)CMSG_DATA(cmsg), sizeof(int));//获取文件描述符
		return 0;
	}

	static int CreateDaemon()//切换到守护进程
	{
		int ret = fork();
		if (ret < 0)return -1;
		if (ret > 0)exit(0);//结束父进程
		ret = setsid();//设置会话组
		if (ret == -1)return -2;
		ret = fork();
		if (ret < 0)return -1;
		if (ret > 0)exit(0);//结束子进程
		for (int i = 0; i < 3; i++)close(i);//关闭stdin stdout stderr
		umask(0);//设置文件权限掩码 表示按照文件默认权限执行
		signal(SIGCHLD, SIG_IGN);//忽略子进程发生变化发信号给父进程
	}
private:
	FuncBase* m_func;
	pid_t m_pid;
	int pipes[2];
};

int CreateLogServer(CProcess* ProcLog)
{
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
	//CProcess::CreateDaemon();
	CProcess logServer, clientServer;
	int ret = logServer.SetEntryFunc(CreateLogServer, &logServer);
	if (ret == -1)
	{
		printf("%s(%d):<%s> pid=%d\r\n", __FILE__, __LINE__, __FUNCTION__, getpid());
		return -1;
	}
	logServer.CreateSubProcess();
	ret = clientServer.SetEntryFunc(CreateClientServer, &clientServer);
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
	close(fd);
	return 0;
}