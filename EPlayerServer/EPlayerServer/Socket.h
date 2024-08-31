#pragma once
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <fcntl.h>//�����ļ�����Ȩ��

enum SocketStatus
{
	SOCKETNoClog = 1,//������ ����Ϊ����
	SOCKETServer = 2,//������ ����Ϊ�ͻ���
	SOCKETUdp = 4,//UDP ����ΪTCP
	SOCKETIp = 8,//IP ����Ϊ�����׽���
};

//������
class Buffer :public std::string
{
public:
	Buffer() :std::string() {}
	Buffer(int size) :std::string() { resize(size); }
	Buffer(const std::string& str) :std::string(str) {}
	Buffer(const char* str) :std::string(str) {}
public:
	operator char* () { return (char*)c_str(); }
	operator char* ()const { return (char*)c_str(); }
	operator const char* ()const { return c_str(); }
};

class SockParam
{
public:
	SockParam()
	{
		bzero(&addrin, sizeof(sockaddr_in));
		bzero(&addrun, sizeof(sockaddr_un));
		port = -1;
		attr = 0;//Ĭ���Ƿ����� �ͻ��� UDP
	}
	~SockParam() {}
	SockParam(const SockParam& data)
	{
		if (this == &data)return;
		memcpy(&addrin, &data.addrin, sizeof(sockaddr_in));
		memcpy(&addrun, &data.addrun, sizeof(sockaddr_un));
		ip = data.ip;
		port = data.port;
		attr = data.attr;
	}
	SockParam(const Buffer& ip, short port, int attr = 0)
	{
		this->ip = ip;
		this->port = port;
		addrin.sin_family = AF_INET;
		addrin.sin_addr.s_addr = inet_addr(ip);
		addrin.sin_port = port;
		this->attr = attr;
	}
	SockParam(const Buffer& path, int attr = 0)
	{
		ip = path;
		//printf("%s(%d):<%s> path=%s\r\n", __FILE__, __LINE__, __FUNCTION__, (char*)ip);
		addrun.sun_family = AF_UNIX;
		strcpy(addrun.sun_path, path);
		this->attr = attr;
	}
public:
	SockParam& operator=(const SockParam& data)
	{
		if (this != &data)
		{
			memcpy(&addrin, &data.addrin, sizeof(sockaddr_in));
			memcpy(&addrun, &data.addrun, sizeof(sockaddr_un));
			ip = data.ip;
			port = data.port;
			attr = data.attr;
		}
		return *this;
	}
	sockaddr* Addrin() { return (sockaddr*)&addrin; }
	sockaddr* Addrun() { return (sockaddr*)&addrun; }
public:
	//��ַ
	sockaddr_in addrin;
	sockaddr_un addrun;
	//ip
	Buffer ip;
	//�˿�
	short port;
	//�Ƿ����� �ο�SockAttr
	int attr;
};

class SocketBase
{
public:
	SocketBase()
	{
		m_socket = -1;
		m_status = 0;
	}
	//����������֤����ָ��ָ��Ķ����ִ���������������
	virtual ~SocketBase()
	{
		Close();
	}
public:
	virtual int Init(const SockParam& param) = 0;//��ʼ���������� ���������׽��֡����׽��֡������׽��� �ͻ��� �����׽���
	virtual int Link(SocketBase** pClient = NULL) = 0;//���� ������ accept �ͻ��� connect udp���Ժ���
	virtual int Send(const Buffer& data) = 0;//��������
	virtual int Recv(Buffer& data) = 0;//��������
	virtual void Close()//�ر�����
	{
		m_status = 3;
		if (m_socket != -1)
		{
			//���׽���Ϊ������ �����׽���ʱ��ɾ������·���ļ�
			if ((m_param.attr & SOCKETServer) && ((m_param.attr & SOCKETIp) == 0))
				unlink("./log/server.sock");
			int tmp = m_socket;
			m_socket = -1;
			close(tmp);
		}
	}
public:
	virtual operator int() { return m_socket; }
	virtual operator int()const { return m_socket; }
protected:
	int m_socket;//�׽��������� Ĭ��-1
	//�׽���״̬
	//0 δ��ʼ��
	//1 ��ʼ��
	//2 ������
	//3 �ر�����
	int m_status;
	SockParam m_param;//�׽��ֲ���
};

class CSocket :public SocketBase
{
public:
	CSocket() :SocketBase() {}
	CSocket(int fd) :SocketBase()
	{
		m_socket = fd;
	}
	~CSocket()
	{
		Close();
	}
public:
	virtual int Init(const SockParam& param)//��ʼ���������� ���������׽��֡����׽��֡������׽��� �ͻ��� �����׽���
	{
		if (m_status > 0)return -1;
		m_param = param;
		if (m_socket == -1)
		{
			int type = m_param.attr & SOCKETUdp ? SOCK_DGRAM : SOCK_STREAM;
			if (m_param.attr & SOCKETIp)
				m_socket = socket(PF_INET, type, 0);
			else
				m_socket = socket(PF_LOCAL, type, 0);
			if (m_socket == -1)return -2;
		}
		else
		{
			m_status = 2;//��accpet���׽��ֽ��г�ʼ��������Ҫ���´����׽��֣�ֻ��Ҫ����״̬
		}
		int ret = 0;
		//�������׽��ִ����߼� bind listen
		if (param.attr & SOCKETServer)
		{
			if (m_param.attr & SOCKETIp)
				ret = bind(m_socket, m_param.Addrin(), sizeof(sockaddr_in));
			else
				ret = bind(m_socket, m_param.Addrun(), sizeof(sockaddr_un));
			if (ret == -1)
			{
				Close();
				return -3;
			}
			ret = listen(m_socket, 32);
			if (ret == -1)
			{
				Close();
				return -4;
			}
		}
		//���÷�����
		if (param.attr & SOCKETNoClog)
		{
			int option = fcntl(m_socket, F_GETFL);
			if (option == -1)return -5;
			option |= O_NONBLOCK;
			ret = fcntl(m_socket, F_SETFL, option);
			if (ret == -1)return -6;
		}
		if (m_status == 0)
			m_status = 1;
		return 0;
	}
	virtual int Link(SocketBase** pClient = NULL)//���� ������ accept �ͻ��� connect udp���Ժ���
	{
		//�Ѿ���ʼ�������������״̬�� ���׽���δ��ʼ��
		if ((m_status == 0 || m_status == 3) || m_socket == -1)return -1;
		if (m_param.attr & SOCKETUdp)return 0;
		int ret = 0;
		if (m_param.attr & SOCKETServer)
		{
			if (pClient == NULL)return -2;
			SockParam param;
			int fd;
			if (m_param.attr & SOCKETIp)
			{
				param.attr |= SOCKETIp;
				socklen_t len = sizeof(sockaddr_in);
				fd = accept(m_socket, param.Addrin(), &len);
			}
			else
			{
				socklen_t len = sizeof(sockaddr_un);
				fd = accept(m_socket, param.Addrun(), &len);
			}
			if (fd == -1)return -3;
			*pClient = new CSocket(fd);
			if ((*pClient) == NULL)return -4;
			ret = (*pClient)->Init(param);
			if (ret != 0)
			{
				delete (*pClient);
				*pClient = NULL;
				return -5;
			}
		}
		else
		{
			if (m_param.attr & SOCKETIp)
				ret = connect(m_socket, m_param.Addrin(), sizeof(sockaddr_in));
			else
				ret = connect(m_socket, m_param.Addrun(), sizeof(sockaddr_un));
			if (ret == -1)return -6;
		}
		m_status = 2;
		return 0;
	}
	virtual int Send(const Buffer& data)//��������
	{
		if (m_status != 2 || m_socket == -1)return -1;
		ssize_t index = 0;
		while (index < (ssize_t)data.size())
		{
			ssize_t len = write(m_socket, (char*)data + index, data.size() - index);
			if (len < 0)return -2;
			if (len == 0)return -3;
			index += len;
		}
		return 0;
	}
	virtual int Recv(Buffer& data)//��������
	{
		if (m_status != 2 || m_socket == -1)return -1;
		ssize_t len = read(m_socket, data, data.size());
		if (len > 0)
		{
			data.resize(len);
			return (int)len;//���ؽ��ճ���
		}
		if (len < 0)
		{
			if ((errno == EINTR) || (errno == EAGAIN))
			{
				data.clear();
				return 0;
			}
			return -2;//û�н��յ�����
		}
		return -3;//�׽��ֹر�
	}
	virtual void Close()//�ر�����
	{
		return SocketBase::Close();
	}
};