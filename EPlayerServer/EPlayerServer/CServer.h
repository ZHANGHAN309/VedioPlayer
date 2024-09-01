#pragma once
#include "Process.h"
#include "Epoll.h"
#include "ThreadPool.h"
#include "Logger.h"
#include "Socket.h"

//特例化
template<typename _FUNC_, typename... _ARG_>
class CConnectFunction :public FuncBase
{
public:
	CConnectFunction(_FUNC_ func, _ARG_... args)
		:m_binder(std::forward<_FUNC_>(func), std::forward<_ARG_>(args)...)//std::forward表示原样转发
	{
	}
	virtual ~CConnectFunction() {}
	virtual int operator()(SocketBase* pClient)
	{
		return m_binder(pClient);
	}
private:
	typename std::_Bindres_helper<int, _FUNC_, _ARG_...>::type m_binder;
};

template<typename _FUNC_, typename... _ARG_>
class CRecvFunction :public FuncBase
{
public:
	CRecvFunction(_FUNC_ func, _ARG_... args)
		:m_binder(std::forward<_FUNC_>(func), std::forward<_ARG_>(args)...)//std::forward表示原样转发
	{
	}
	virtual ~CRecvFunction() {}
	virtual int operator()(SocketBase* pClient, const Buffer& data)
	{
		return m_binder(pClient, data);
	}
private:
	typename std::_Bindres_helper<int, _FUNC_, _ARG_...>::type m_binder;
};

class CBusinessBase
{
public:
	CBusinessBase() { m_connected = NULL; m_recv = NULL; }
	virtual int BusinessProcess(CProcess* proc) = 0;
	template<typename _FUNC_, typename... _ARG_>
	int setConnected(_FUNC_ funcName, _ARG_... args);
	template<typename _FUNC_, typename... _ARG_>
	int setRecv(_FUNC_ funcName, _ARG_... args);
protected:
	FuncBase* m_connected;
	FuncBase* m_recv;
};

class CServer
{
public:
	CServer();
	~CServer() { Close(); }
public:
	CServer(const CServer&) = delete;
	CServer& operator=(const CServer&) = delete;
public:
	int Init(CBusinessBase* business,const Buffer& ip="127.0.0.1",short port = 9999);
	int Run();
	void Close();
private:
	int ThreadFunc();
private:
	CEpoll m_epoll;
	SocketBase* m_server;
	ThreadPool m_pool;
	CProcess m_process;
	CBusinessBase* m_pBusiness;//业务模块 需要手动delete
};

template<typename _FUNC_, typename ..._ARG_>
inline int CBusinessBase::setConnected(_FUNC_ funcName, _ARG_ ...args)
{
	m_connected = new CConnectFunction<_FUNC_, _ARG_...>(funcName, args...);
	if (m_connected == NULL)return -1;
	return 0;
}

template<typename _FUNC_, typename ..._ARG_>
inline int CBusinessBase::setRecv(_FUNC_ funcName, _ARG_ ...args)
{
	m_recv = new CRecvFunction<_FUNC_, _ARG_...>(funcName, args...);
	if (m_recv == NULL)return -1;
	return 0;
}
