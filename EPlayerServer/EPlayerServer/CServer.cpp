#include "CServer.h"

CServer::CServer()
{
	m_server = NULL;
	m_pBusiness = NULL;
}

int CServer::Init(CBusinessBase* business, const Buffer& ip, short port)
{
	if (m_server != NULL)return -1;
	if (business == NULL)return -2;
	//创建客户端处理进程
	m_pBusiness = business;
	int ret = 0;
	ret = m_process.SetEntryFunc(&CBusinessBase::BusinessProcess, m_pBusiness, &m_process);
	if (ret != 0)return -3;
	ret = m_process.CreateSubProcess();
	if (ret != 0)return -4;
	//创建服务器套接字
	m_server = new CSocket();
	if (m_server == NULL)return -5;
	ret = m_server->Init(SockParam(ip, port, SOCKETServer | SOCKETIp));
	if (ret != 0)return -6;
	ret = m_server->Link();
	if (ret != 0)return -7;
	//初始化epoll
	ret = m_epoll.Create(2);
	if (ret != 0)return -8;
	ret = m_epoll.Add(*m_server, EPdata((void*)m_server));
	if (ret != 0)return -9;
	//初始化线程池
	ret = m_pool.Start(2);
	if (ret != 0)return -10;
	for (size_t i = 0; i < m_pool.Size(); i++)
	{
		ret = m_pool.AddTask(&CServer::ThreadFunc, this);
		if (ret != 0)return -11;
	}
	return 0;
}

int CServer::Run()
{
	while (m_server != NULL)
	{
		usleep(10);
	}
	return 0;
}

void CServer::Close()
{
	if (m_server != NULL)
	{
		SocketBase* tmp = m_server;
		m_server = NULL;
		m_epoll.Del(*tmp);
		delete tmp;
	}
	m_process.SendFD(-1);
	m_epoll.Close();
	m_pool.Close();
	//delete m_pBusiness;//可能会导致未定义行为
}

int CServer::ThreadFunc()
{
	int ret = 0;
	EPevents events;
	while ((m_server != NULL) && (m_epoll != -1))
	{
		ssize_t eSize = m_epoll.WaitForEpoll(events, 10);
		if (eSize < 0)break;
		if (eSize > 0)
		{
			for (ssize_t i = 0; i < eSize; i++)
			{
				if (events[i].events & EPOLLERR)break;
				else if (events[i].events & EPOLLIN)
				{
					if (events[i].data.ptr == m_server)
					{
						if (m_server)
						{
							SocketBase* client = NULL;
							ret = m_server->Link(&client);
							if (ret != 0)continue;
							ret = m_process.SendSock(*client, *client);
							if (ret != 0)
								TRACEE("recv client %d failed!\r\n", (int)*client);
							delete client;
						}
					}
				}
			}
		}
	}
	return 0;
}
