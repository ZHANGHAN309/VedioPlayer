#include "ThreadPool.h"

int ThreadPool::Start(unsigned count)
{
	if (m_server != NULL)return -1;
	if (m_path.size() == 0)return -2;//³õÊ¼»¯Ê§°Ü
	m_server = new CSocket();
	if (m_server == NULL)return -3;
	int ret = m_server->Init(SockParam(m_path, SOCKETServer));
	if (ret != 0)return -4;
	ret = m_epoll.Create(count);
	if (ret != 0)return -5;
	ret = m_epoll.Add(*m_server, EPdata((void*)m_server));
	if (ret != 0)return -6;
	m_pool.resize(count);
	for (unsigned i = 0; i < count; i++)
	{
		m_pool[i] = new Thread(&ThreadPool::TaskDispatch, this);
		if (m_pool[i] == NULL)return -7;
		ret = m_pool[i]->Start();
		if (ret != 0)return -8;
	}
	return 0;
}

void ThreadPool::Close()
{
	m_epoll.Close();
	if (m_server != NULL)
	{
		SocketBase* tmp = m_server;
		m_server = NULL;
		delete tmp;
	}
	for (auto thread : m_pool)
	{
		if (thread)delete thread;
	}
	m_pool.clear();
	unlink(m_path);
}

int ThreadPool::TaskDispatch()
{
	if (m_server == NULL)return -1;
	EPevents events;
	int ret = 0;
	while (m_epoll != -1)
	{
		ssize_t esize = m_epoll.WaitForEpoll(events, 1);
		if (esize > 0)
		{
			for (ssize_t i = 0; i < esize; i++)
			{
				if (events[i].events & EPOLLIN)
				{
					SocketBase* pClient = NULL;
					if (events[i].data.ptr == m_server)
					{
						ret = m_server->Link(&pClient);
						if (ret != 0)continue;
						ret = m_epoll.Add(*pClient, EPdata((void*)pClient));
						if (ret != 0)
						{
							delete pClient;
							continue;
						}
					}
					else
					{
						pClient = (SocketBase*)events[i].data.ptr;
						if (pClient == NULL)continue;
						FuncBase* base = NULL;
						Buffer data(sizeof(base));
						ret = pClient->Recv(data);
						if (ret <= 0)
						{
							m_epoll.Del(*pClient);
							delete pClient;
							continue;
						}
						memcpy(&base, (char*)data, sizeof(base));
						if (base != NULL)
						{
							(*base)();
							delete base;
						}
					}
				}
			}
		}
	}
	return 0;
}
