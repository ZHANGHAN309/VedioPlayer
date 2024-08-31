#pragma once
#include <vector>
#include "Thread.h"
#include "Epoll.h"
#include "Socket.h"
#include "Logger.h"

class ThreadPool
{
public:
	ThreadPool() 
	{
		m_server = NULL;
		timespec tp = { 0,0 };//Linux����ʱ��ṹ��
		clock_gettime(CLOCK_REALTIME, &tp);//��ȡ��ǰʱ��
		char* buf = NULL;
		asprintf(&buf, "%d.%d.sock", tp.tv_sec % 10000, tp.tv_nsec % 1000000);//ͨ��ʱ�������ļ�
		if (buf != NULL)
		{
			m_path = buf;
			free(buf);//asprintf()������ϵͳ�Զ�Ϊbuf�����ڴ棬������Ҫ�ֶ�����
		}
		usleep(1);//���߷�ֹ��pool��ʼ����ʱ��������ͬ�ļ�
	}
	~ThreadPool() 
	{
		Close();
	}
	int Start(unsigned count);
	void Close();
	template<typename _FUNC_,typename... _ARGS_>
	int AddTask(_FUNC_ func,_ARGS_... args);
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
private:
	int TaskDispatch();
private:
	std::vector<Thread*> m_pool;
	CEpoll m_epoll;
	SocketBase* m_server;
	Buffer m_path;
};

template<typename _FUNC_, typename ..._ARGS_>
inline int ThreadPool::AddTask(_FUNC_ func, _ARGS_ ...args)
{
	static thread_local CSocket client;
	int ret = 0;
	if (client == -1)
	{
		ret = client.Init(SockParam(m_path));
		if (ret != 0)return -1;
		ret = client.Link();
		if (ret != 0)return -2;
	}
	FuncBase* base = new CFunction<_FUNC_, _ARGS_...>(func, args...);
	if (base == NULL)return -3;
	Buffer data(sizeof(base));
	memcpy(data, &base, sizeof(base));
	ret = client.Send(data);
	if (ret != 0)
	{
		delete base;
		return -4;
	}
	return 0;
}
