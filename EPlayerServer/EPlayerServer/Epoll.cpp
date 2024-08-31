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
	if (m_epoll != -1)return -1;//epoll�׽����Ѿ�������
	m_epoll = epoll_create(size);
	if (m_epoll == -1)return -2;
	return 0;
}

ssize_t CEpoll::WaitForEpoll(EPevents& events, int timeout)
{
	if (m_epoll == -1)return -1;
	EPevents evt(EPOLL_SIZE);//�����¼�����
	int ret = epoll_wait(m_epoll, evt.data(), (int)evt.size(), timeout);
	if (ret == -1)
	{
		if ((errno == EINTR) || (errno == EAGAIN))//�ж�ϵͳ��Ӧ������ �����жϳ����ж�epoll_wait������-1 errnoΪEINTR
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


