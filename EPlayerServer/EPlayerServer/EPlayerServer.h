#pragma once
#include "CServer.h"
#include <map>

#define ERR_RETURN(ret,num) if(ret!=0){TRACEE("ret=%d errno=%d msg=%d\r\n",ret,errno,strerror(errno));return num;}
#define WARNING_CONTINUE(ret) if(ret!=0){TRACEW("ret=%d errno=%d msg=%d\r\n",ret,errno,strerror(errno));}

class EPlayerServer :public CBusinessBase
{
public:
	EPlayerServer(unsigned count) :CBusinessBase()
	{
		m_count = count;
	}
	~EPlayerServer()
	{
		m_epoll.Close();
		m_pool.Close();
		for (auto client : m_mapClients)
		{
			if (client.second)delete client.second;
		}
	}
	virtual int BusinessProcess(CProcess* proc)
	{
		using namespace std::placeholders;
		int ret = 0;
		ret = setConnected(&EPlayerServer::Connected, this, _1);
		ERR_RETURN(ret, -1);
		ret = setRecv(&EPlayerServer::Received, this, _1, _2);
		ERR_RETURN(ret, -1);
		ret = m_epoll.Create(m_count);
		ERR_RETURN(ret, -1);
		ret = m_pool.Start(m_count);
		ERR_RETURN(ret, -2);
		for (unsigned i = 0; i < m_count; i++)
		{
			ret = m_pool.AddTask(&EPlayerServer::ThreadFunc, this);
			ERR_RETURN(ret, -3);
		}
		int sock = 0;
		sockaddr_in addrin;
		while (m_epoll != -1)
		{
			ret = proc->RecvSock(sock, &addrin);
			if (ret != 0 && sock <= 0)break;
			SocketBase* pClient = new CSocket(sock);
			if (pClient == NULL)continue;
			ret = pClient->Init(SockParam(&addrin, SOCKETIp));
			WARNING_CONTINUE(ret);
			ret = m_epoll.Add(*pClient, EPdata((void*)pClient));
			WARNING_CONTINUE(ret);
			if (m_connected)
			{
				(*m_connected)();//需要传递什么参数
			}
		}
		return 0;
	}
private:
	int ThreadFunc()
	{
		int ret = 0;
		EPevents events;
		while (m_epoll != -1)
		{
			ssize_t eSize = m_epoll.WaitForEpoll(events);
			if (eSize < 0)break;
			if (eSize > 0)
			{
				for (ssize_t i = 0; i < eSize; i++)
				{
					if (events[i].events & EPOLLERR)break;
					else if (events[i].events & EPOLLIN)
					{
						SocketBase* pClient = (SocketBase*)events[i].data.ptr;
						if (pClient == NULL)continue;
						Buffer data;
						ret = pClient->Recv(data);
						WARNING_CONTINUE(ret);
						if (m_recv)
						{
							(*m_recv)();//需要传递什么参数
						}
					}
				}
			}
		}
		return 0;
	}
	int Connected(SocketBase* pClient)
	{
		return 0;
	}
	int Received(SocketBase* pClient, const Buffer& data)
	{
		return 0;
	}
private:
	CEpoll m_epoll;
	std::map<int, SocketBase*> m_mapClients;
	ThreadPool m_pool;
	unsigned m_count;
};