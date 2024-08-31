#include "Epoll.h"

CEpoll::CEpoll()
{
	m_epoll = -1;
}

CEpoll::~CEpoll()
{
	Close();
}

int CEpoll::Create(int size)
{
	if (m_epoll != -1)return -1;//epoll套接字已经被创建
	m_epoll = epoll_create(size);
	if (m_epoll == -1)return -2;
	return 0;
}

ssize_t CEpoll::WaitForEpoll(EPevents& events, int timeout)
{
	if (m_epoll == -1)return -1;
	EPevents evt(EPOLL_SIZE);//创建事件数组
	int ret = epoll_wait(m_epoll, evt.data(), (int)evt.size(), timeout);
	if (ret == -1)
	{
		if ((errno == EINTR) || (errno == EAGAIN))//中断系统响应或重试 当有中断程序中断epoll_wait，返回-1 errno为EINTR
		{
			return 0;
		}
		return -2;
	}
	if (ret > (int)events.size())
	{
		events.resize(ret);
	}
	memcpy(events.data(), evt.data(), sizeof(epoll_event) * ret);
	return ret;
}

int CEpoll::Add(int fd, const EPdata& data, uint32_t events)
{
	if (m_epoll == -1)return -1;
	epoll_event tmp = { events,data };
	int ret = epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &tmp);
	if (ret == -1)return -2;
	return 0;
}

int CEpoll::Mod(int fd, uint32_t events, const EPdata& data)
{
	if (m_epoll == -1)return -1;
	epoll_event tmp = { events,data };
	int ret = epoll_ctl(m_epoll, EPOLL_CTL_MOD, fd, &tmp);
	if (ret == -1)return -2;
	return 0;
}

int CEpoll::Del(int fd)
{
	if (m_epoll == -1)return -1;
	int ret = epoll_ctl(m_epoll, EPOLL_CTL_DEL, fd, NULL);
	if (ret == -1)return -2;
	return 0;
}

void CEpoll::Close()
{
	if (m_epoll == -1)return;
	close(m_epoll);
	m_epoll = -1;
}


