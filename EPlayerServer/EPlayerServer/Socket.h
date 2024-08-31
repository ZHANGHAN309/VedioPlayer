#pragma once
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <fcntl.h>//关于文件操作权限

enum SocketStatus
{
	SOCKETNoClog = 1,//非阻塞 否则为阻塞
	SOCKETServer = 2,//服务器 否则为客户端
	SOCKETUdp = 4,//UDP 否则为TCP
	SOCKETIp = 8,//IP 否则为本地套接字
};

//缓冲区
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
		attr = 0;//默认是非阻塞 客户端 UDP
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
	//地址
	sockaddr_in addrin;
	sockaddr_un addrun;
	//ip
	Buffer ip;
	//端口
	short port;
	//是否阻塞 参考SockAttr
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
	//虚析构，保证基类指针指向的对象会执行子类的析构函数
	virtual ~SocketBase()
	{
		Close();
	}
public:
	virtual int Init(const SockParam& param) = 0;//初始化，服务器 包括创建套接字、绑定套接字、监听套接字 客户端 创建套接字
	virtual int Link(SocketBase** pClient = NULL) = 0;//链接 服务器 accept 客户端 connect udp可以忽略
	virtual int Send(const Buffer& data) = 0;//发送数据
	virtual int Recv(Buffer& data) = 0;//接收数据
	virtual void Close()//关闭连接
	{
		m_status = 3;
		if (m_socket != -1)
		{
			//当套接字为服务器 本地套接字时，删除本地路径文件
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
	int m_socket;//套接字描述符 默认-1
	//套接字状态
	//0 未初始化
	//1 初始化
	//2 已连接
	//3 关闭连接
	int m_status;
	SockParam m_param;//套接字参数
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
	virtual int Init(const SockParam& param)//初始化，服务器 包括创建套接字、绑定套接字、监听套接字 客户端 创建套接字
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
			m_status = 2;//将accpet的套接字进行初始化，不需要重新创建套接字，只需要设置状态
		}
		int ret = 0;
		//服务器套接字处理逻辑 bind listen
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
		//设置非阻塞
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
	virtual int Link(SocketBase** pClient = NULL)//链接 服务器 accept 客户端 connect udp可以忽略
	{
		//已经初始化后或正常连接状态下 或套接字未初始化
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
	virtual int Send(const Buffer& data)//发送数据
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
	virtual int Recv(Buffer& data)//接收数据
	{
		if (m_status != 2 || m_socket == -1)return -1;
		ssize_t len = read(m_socket, data, data.size());
		if (len > 0)
		{
			data.resize(len);
			return (int)len;//返回接收长度
		}
		if (len < 0)
		{
			if ((errno == EINTR) || (errno == EAGAIN))
			{
				data.clear();
				return 0;
			}
			return -2;//没有接收到数据
		}
		return -3;//套接字关闭
	}
	virtual void Close()//关闭连接
	{
		return SocketBase::Close();
	}
};