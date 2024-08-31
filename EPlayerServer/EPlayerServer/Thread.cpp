#include "Thread.h"
std::map<pthread_t, Thread*> Thread::m_ThreadMap;

int Thread::Start()
{
	pthread_attr_t attr;//不知道是什么？线程参数？
	int ret = 0;
	ret = pthread_attr_init(&attr);
	if (ret != 0)return -1;
	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (ret != 0)return -2;
	/*ret = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
	if (ret != 0)return -3;*/
	ret = pthread_create(&m_threadId, &attr, &Thread::ThreadEntry, this);
	if (ret != 0)return -4;
	m_ThreadMap[m_threadId] = this;
	m_IsHover = false;
	ret = pthread_attr_destroy(&attr);
	if (ret != 0)return -5;
	return 0;
}

int Thread::Stop()
{
	if (m_threadId != 0)
	{
		pthread_t thread = m_threadId;
		m_threadId = 0;
		timespec time;
		time.tv_sec = 0;
		time.tv_nsec = 1e8;//100ms
		int ret = pthread_timedjoin_np(thread, NULL, &time);
		if (ret == ETIMEDOUT)
		{
			pthread_detach(thread);
			pthread_kill(thread, SIGUSR2);
		}
	}
	return 0;
}

int Thread::Pause()
{
	if (m_threadId == 0)return -1;
	if (m_IsHover)return 0;
	m_IsHover = true;
	int ret = pthread_kill(m_threadId, SIGUSR1);//发送信号
	if (ret != 0)return -2;
	return 0;
}

void* Thread::ThreadEntry(void* arg)
{
	Thread* thiz = (Thread*)arg;
	struct sigaction act = { 0 };
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = &Thread::Sigaction;
	sigaction(SIGUSR1, &act, NULL);
	sigaction(SIGUSR2, &act, NULL);
	thiz->EntryThread();

	if (thiz->m_threadId)thiz->m_threadId = 0;
	auto it = m_ThreadMap.find(pthread_self());
	if (it != m_ThreadMap.end())
	{
		m_ThreadMap[pthread_self()] = NULL;
	}
	pthread_detach(pthread_self());
	pthread_exit(NULL);
}

void Thread::Sigaction(int signo, siginfo_t* info, void* context)
{
	if (signo == SIGUSR1)
	{
		//暂停线程
		pthread_t thread = pthread_self();
		auto it = m_ThreadMap.find(thread);
		if (it != m_ThreadMap.end())
		{
			if (it->second)
			{
				it->second->m_IsHover = true;
				while (it->second->m_IsHover)
				{
					if (it->second->m_threadId == 0)
					{
						pthread_exit(NULL);
					}
					usleep(1000);
				}
			}
		}
	}
	else if (signo == SIGUSR2)
	{
		//关闭线程
		pthread_exit(NULL);
	}
}

void Thread::EntryThread()
{
	if (m_func != NULL)
	{
		int ret = (*m_func)();//执行函数
		if (ret != 0)
		{
			printf("%s(%d):<%s> ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
		}
	}
}
