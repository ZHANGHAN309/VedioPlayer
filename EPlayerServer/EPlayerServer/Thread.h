#pragma once
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <map>
#include "Function.h"
#include <fcntl.h>

class Thread
{
public:
	Thread() { m_func = NULL; m_threadId = 0; m_IsHover = false; }
	template<typename _FUNC_, typename... _ARG_>
	Thread(_FUNC_ funcName, _ARG_... arg) :m_func{ new CFunction<_FUNC_,_ARG_...>(funcName,arg...) } 
	{
		m_threadId = 0;
		m_IsHover = false; 
	}
	~Thread() {}
	//�����̺߳���
	template<typename _FUNC_, typename... _ARG_>
	int SetThreadFunc(_FUNC_ funcName, _ARG_... arg)
	{
		m_func = new CFunction<_FUNC_, _ARG_...>(funcName, arg...);
		if (m_func == NULL)return -1;
		return 0;
	}
	//�����߳�
	int Start();
	
	int Stop();
	
	int Pause();
	
	bool isValid() { return m_threadId != 0; }
public:
	//�߳��ǲ��ܸ��Ƶ�
	Thread(const Thread&) = delete;
	Thread& operator=(const Thread&) = delete;
public:
	//__stdcall �߳̽ӿں���
	static void* ThreadEntry(void* arg);

	static void Sigaction(int signo, siginfo_t* info, void* context);
	
	//__thiscall �߳���ں���
	void EntryThread();
	
private:
	FuncBase* m_func;//ģ�庯�������ָ��
	pthread_t m_threadId;//�߳�tid
	bool m_IsHover;//�߳��Ƿ񱻹�����ͣ����true ��ͣ false ����
	static std::map<pthread_t, Thread*> m_ThreadMap;//�̵߳�ӳ�������������ʱɾ�������߳�
};

