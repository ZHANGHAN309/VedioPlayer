#pragma once
#include "Process.h"
#include "Thread.h"
#include "Socket.h"
#include "Epoll.h"
#include <sys/timeb.h>
#include <stdarg.h>
#include <sstream>
#include <sys/stat.h>

enum LogLevel
{
	LOGINFO,
	LOGDEBUG,
	LOGWARNING,
	LOGERROR,
	LOGFATAL
};

class LogInfo
{
public:
	LogInfo(const char* file,int line,const char* funcname,
		pid_t pid,pthread_t tid,LogLevel level,
		const char* fmt,...);
	LogInfo(const char* file, int line, const char* funcname,
		pid_t pid, pthread_t tid, LogLevel level);
	LogInfo(const char* file, int line, const char* funcname,
		pid_t pid, pthread_t tid, LogLevel level,
		void* pData,size_t nSize);
	~LogInfo();
public:
	operator Buffer()const;
	template<typename T>
	LogInfo& operator<<(const T& data);
public:
	LogInfo() = delete;
private:
	Buffer m_buf;
	bool isAuto;//�Զ�����
};

//log��־����
class Logger
{
public:
	Logger();
	~Logger();
	int Start();
	void Close();
	//�ṩ�ӿڸ��������̻��߳�ʹ��
	static void Trace(const LogInfo& info);
	static Buffer GetTimeStr();
private:
	//�߳�ִ�к���=main
	int ThreadFunc();
	void WriteLog(const Buffer& data);
public:
	//�����ǲ��ܸ��Ƶģ�����ɾ��
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;
private:
	Thread m_thread;
	SocketBase* m_server;
	CEpoll m_epoll;
	Buffer m_path;
	FILE* m_file;
};

#ifndef TRACE
#define TRACEI(...) Logger::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOGINFO,__VA_ARGS__))
#define TRACED(...) Logger::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOGDEBUG,__VA_ARGS__))
#define TRACEW(...) Logger::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOGWARNING,__VA_ARGS__))
#define TRACEE(...) Logger::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOGERROR,__VA_ARGS__))
#define TRACEF(...) Logger::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOGFATAL,__VA_ARGS__))

//���log<<"Hello World"<<"end";
#define LOGI LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOGINFO)
#define LOGD LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOGDEBUG)
#define LOGW LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOGWARNING)
#define LOGE LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOGERROR)
#define LOGF LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOGFATAL)

//01 02 65 ... 01 02 a �ڴ����
#define DUMPI(data,size) Logger::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOGINFO,data,size))
#define DUMPD(data,size) Logger::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOGDEBUG,data,size))
#define DUMPW(data,size) Logger::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOGWARNING,data,size))
#define DUMPE(data,size) Logger::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOGERROR,data,size))
#define DUMPF(data,size) Logger::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOGFATAL,data,size))
#endif // TRACE

template<typename T>
inline LogInfo& LogInfo::operator<<(const T& data)
{
	std::stringstream stream;
	stream << data;
	m_buf += stream.str();
	
	return *this;
}
