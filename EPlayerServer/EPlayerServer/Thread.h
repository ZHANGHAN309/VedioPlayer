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
	//设置线程函数
	template<typename _FUNC_, typename... _ARG_>
	int SetThreadFunc(_FUNC_ funcName, _ARG_... arg)
	{
		m_func = new CFunction<_FUNC_, _ARG_...>(funcName, arg...);
		if (m_func == NULL)return -1;
		return 0;
	}
	//启动线程
	int Start();
	
	int Stop();
	
	int Pause();
	
	bool isValid() { return m_threadId != 0; }
public:
	//线程是不能复制的
	Thread(const Thread&) = delete;
	Thread& operator=(const Thread&) = delete;
public:
	//__stdcall 线程接口函数
	static void* ThreadEntry(void* arg);

	static void Sigaction(int signo, siginfo_t* info, void* context);
	
	//__thiscall 线程入口函数
	void EntryThread();
	
private:
	FuncBase* m_func;//模板函数基类的指针
	pthread_t m_threadId;//线程tid
	bool m_IsHover;//线程是否被挂起（暂停），true 暂停 false 运行
	static std::map<pthread_t, Thread*> m_ThreadMap;//线程的映射表，方便程序结束时删除所有线程
};

