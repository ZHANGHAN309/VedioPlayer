#pragma once
#include "Function.h"
#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>

class CProcess
{
public:
	CProcess()
	{
		m_func = NULL;
		memset(pipes, 0, sizeof(pipes));
		m_pid = 0;
	}
	~CProcess()
	{
		//这个析构函数貌似只有主进程会调用
		if (m_func != NULL)
		{
			delete(m_func);
			m_func = NULL;
		}
		close(pipes[m_pid]);
		pipes[m_pid] = 0;
	}
	template<typename _FUNC_, typename... _ARG_>
	int SetEntryFunc(_FUNC_ funcName, _ARG_... args)
	{
		if (m_func != NULL)return -1;
		m_func = new CFunction<_FUNC_, _ARG_...>(funcName, args...);
		return 0;
	}

	int CreateSubProcess()
	{
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
			m_pid = 1;
			(*m_func)();//子进程
			close(pipes[1]);
			pipes[1] = 0;
			exit(0);
		}
		printf("%s(%d):<%s> pid=%d\r\n", __FILE__, __LINE__, __FUNCTION__, getpid());
		close(pipes[1]);//主进程用第1个套接字
		pipes[1] = 0;
		m_pid = 0;//主进程
		printf("%s(%d):<%s> main process pid=%d\r\n", __FILE__, __LINE__, __FUNCTION__, m_pid);
		return 0;
	}
	//发送文件描述符
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
		printf("%s(%d):<%s> fd=%d pid=%d pipes[0]=%d\r\n", __FILE__, __LINE__, __FUNCTION__, fd, getpid(), pipes[0]);
		*(int*)CMSG_DATA(cmsg) = (int)fd;
		msg.msg_control = cmsg;
		msg.msg_controllen = CMSG_LEN(sizeof(int));
		int ret = (int)sendmsg(pipes[0], &msg, 0);
		printf("%s(%d):<%s> ret=%d pid=%d pipes[0]=%d\r\n", __FILE__, __LINE__, __FUNCTION__, ret, getpid(), pipes[m_pid]);
		free(cmsg);
		if (ret <= 0)
		{
			perror("Sendmsg failed!\r\n");
			return -2;
		}
		return 0;
	}
	//接收文件描述符
	int RecvFD(int& fd)
	{
		msghdr msg;
		iovec iov;
		char buffer[13] = "";
		iov.iov_base = buffer;
		iov.iov_len = 13;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));
		if (cmsg == NULL)return -1;
		cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		msg.msg_control = cmsg;
		msg.msg_controllen = CMSG_LEN(sizeof(int));
		ssize_t ret = recvmsg(pipes[1], &msg, 0);
		if (ret == -1) {
			free(cmsg);
			perror("Recv msg failed!\r\n");
			return -2;
		}
		fd = *(int*)CMSG_DATA(cmsg);
		printf("%s(%d):<%s> fd=%d pid=%d pipes[1]=%d\r\n", __FILE__, __LINE__, __FUNCTION__, fd, getpid(), pipes[m_pid]);
		free(cmsg);
		return 0;
	}
	//int RecvFD(int& fd)
	//{
	//	iovec iov;
	//	char buffer[13] = "";
	//	iov.iov_base = buffer;
	//	iov.iov_len = 13;
	//	msghdr msg;
	//	bzero(&msg, sizeof(msg));
	//	msg.msg_iov = &iov;//设置缓冲区
	//	msg.msg_iovlen = 1;
	//	char control_data[CMSG_SPACE(sizeof(int))] = { 0 };
	//	msg.msg_control = control_data;//设置控制信息
	//	msg.msg_controllen = sizeof(control_data);
	//	int ret = (int)recvmsg(pipes[1], &msg, 0);//接收信息
	//	if (ret <= 0)
	//	{
	//		perror("Recvmsg failed!\r\n");
	//		return -2;
	//	}
	//	cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);//获取控制信息
	//	printf("%s(%d):<%s> cmsg:%08X\r\n", __FILE__, __LINE__, __FUNCTION__, cmsg);
	//	//free(cmsg);
	//	if (cmsg == NULL)
	//	{
	//		perror("Find first cmsghdr failed!\r\n");
	//		return -1;
	//	}
	//	memcpy(&fd, (int*)CMSG_DATA(cmsg), sizeof(int));//获取文件描述符
	//	return 0;
	//}

	int SendSocket(int fd, const sockaddr_in* addrin) {//主进程完成
		struct msghdr msg;
		iovec iov;
		char buf[20] = "";
		bzero(&msg, sizeof(msg));
		memcpy(buf, addrin, sizeof(sockaddr_in));
		iov.iov_base = buf;
		iov.iov_len = sizeof(buf);
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		// 下面的数据，才是我们需要传递的。
		cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));
		if (cmsg == NULL)return -1;
		cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		*(int*)CMSG_DATA(cmsg) = fd;
		msg.msg_control = cmsg;
		msg.msg_controllen = cmsg->cmsg_len;

		ssize_t ret = sendmsg(pipes[1], &msg, 0);
		free(cmsg);
		if (ret == -1) {
			printf("********errno %d msg:%s\n", errno, strerror(errno));
			return -2;
		}
		return 0;
	}

	int RecvSocket(int& fd, sockaddr_in* addrin)
	{
		msghdr msg;
		iovec iov;
		char buf[20] = "";
		bzero(&msg, sizeof(msg));
		iov.iov_base = buf;
		iov.iov_len = sizeof(buf);
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));
		if (cmsg == NULL)return -1;
		cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		msg.msg_control = cmsg;
		msg.msg_controllen = CMSG_LEN(sizeof(int));
		ssize_t ret = recvmsg(pipes[0], &msg, 0);
		if (ret == -1) {
			free(cmsg);
			return -2;
		}
		memcpy(addrin, buf, sizeof(sockaddr_in));
		fd = *(int*)CMSG_DATA(cmsg);
		free(cmsg);
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
	pid_t m_pid;//主进程pork后为0，子进程为1
	int pipes[2];
};