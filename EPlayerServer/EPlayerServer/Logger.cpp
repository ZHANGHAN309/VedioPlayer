#include "Logger.h"

Logger::Logger()
	:m_thread(&Logger::ThreadFunc, this)
{
	m_server = NULL;
	m_path = "./log/";
	Buffer time = GetTimeStr();
	m_path += time;
#ifdef _Debug
	//printf("%s(%d):<%s> time=%s\r\n", __FILE__, __LINE__, __FUNCTION__, (char*)time);
#endif // _Debug
	m_path += ".log";
#ifdef _Debug
	//printf("%s(%d):<%s> path=%s\r\n", __FILE__, __LINE__, __FUNCTION__, (char*)m_path);
#endif // _Debug
}

Logger::~Logger()
{
	Close();
}

int Logger::ThreadFunc()
{
	EPevents events;
	ssize_t ret = 0;
	std::map<int, SocketBase*> mapClients;
	while ((m_server != NULL) && (m_epoll != -1) && (m_thread.isValid()))
	{
		ret = m_epoll.WaitForEpoll(events, 1);
		if (ret < 0)break;
		if (ret > 0)
		{
			ssize_t i = 0;
			for (; i < ret; i++)
			{
				if (events[i].events & EPOLLERR)
				{
					break;
				}
				else if (events[i].events & EPOLLIN)
				{
					//服务器的输入进来时，只可能是有新客户端连接进来
					if (events[i].data.ptr == m_server)
					{
						printf("%s(%d):<%s> server epollin pid=%d tid=%d\r\n", __FILE__, __LINE__, __FUNCTION__, events[i].data.ptr, getpid(), pthread_self());
						SocketBase* pClient = NULL;
						int r = m_server->Link(&pClient);
						if (r != 0)continue;
						r = m_epoll.Add(*pClient, EPdata((void*)pClient), EPOLLIN | EPOLLERR);
						if (r != 0)
						{
							delete pClient;
							continue;
						}
						printf("%s(%d):<%s> pid=%d tid=%d\r\n", __FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self());
						//当mapClients里已经存在pClient套接字并且指向的内存不为空时，覆盖该SockerBase指针并回收原来的内存
						auto it = mapClients.find(*pClient);
						if (it != mapClients.end() && it->second != NULL)
						{
							delete it->second;
							mapClients[*pClient] = pClient;
						}
					}
					else
					{
						SocketBase* pClient = (SocketBase*)events[i].data.ptr;
						if (pClient == NULL)continue;
						printf("%s(%d):<%s> client epollin pid=%d tid=%d\r\n", __FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self());
						Buffer data(1024 * 1024);//由于只接收一次，所以当缓冲区过小时可以增大一些
						int r = pClient->Recv(data);
						if (r <= 0)
						{
							mapClients[*pClient] = NULL;
							delete pClient;
						}
						else
						{
							WriteLog(data);
						}
					}
				}
			}
			if (i != ret)break;//当循环提前结束，代表套接字出现问题，应直接结束
		}
	}
	for (auto i = mapClients.begin(); i != mapClients.end(); i++)
	{
		if (i->second)delete i->second;
	}
	mapClients.clear();
	return 0;
}

int Logger::Start()
{
	if (m_server != NULL)return -1;
	if (access("log", W_OK | R_OK) != 0)
	{
		mkdir("log", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	}
	m_file = fopen(m_path, "w+");
	if (m_file == NULL)return -2;
	if (m_server != NULL)return -3;
	int ret = m_epoll.Create(1);
	if (ret != 0)return -4;

	m_server = new CSocket();
	if (m_server == NULL)
	{
		TRACEE("before close\r\n");
		printf("%s(%d):<%s> ret=%d,errno=%d,msg=%s\r\n", __FILE__, __LINE__, __FUNCTION__, ret, errno, strerror(errno));
		Close();
		return -5;
	}
	//解决了一个小bug TvT
	ret = m_server->Init(SockParam("./log/server.sock", (int)(SOCKETServer)));
	//printf("%s(%d):<%s> ret=%d,errno=%d,msg=%s\r\n", __FILE__, __LINE__, __FUNCTION__, ret, errno, strerror(errno));
	if (ret != 0)
	{
		TRACEE("before close ret=%d\r\n", ret);
		printf("%s(%d):<%s> ret=%d,errno=%d,msg=%s\r\n", __FILE__, __LINE__, __FUNCTION__, ret, errno, strerror(errno));
		Close();
		return -6;
	}
	//发现在Init之前Add会失败
	ret = m_epoll.Add(*m_server, EPdata((void*)m_server), EPOLLIN | EPOLLERR);
	//printf("%s(%d):<%s> ret=%d,errno=%d,msg=%s\r\n", __FILE__, __LINE__, __FUNCTION__, ret, errno, strerror(errno));
	if (ret != 0)
	{
		TRACEE("before close ret=%d\r\n", ret);
		printf("%s(%d):<%s> ret=%d,errno=%d,msg=%s\r\n", __FILE__, __LINE__, __FUNCTION__, ret, errno, strerror(errno));
		Close();
		return -7;
	}
	ret = m_thread.Start();
	//printf("%s(%d):<%s> ret=%d,errno=%d,msg=%s\r\n", __FILE__, __LINE__, __FUNCTION__, ret, errno, strerror(errno));
	if (ret != 0)
	{
		TRACEE("before close ret=%d\r\n", ret);
		printf("%s(%d):<%s> ret=%d,errno=%d,msg=%s\r\n", __FILE__, __LINE__, __FUNCTION__, ret, errno, strerror(errno));
		Close();
		return -8;
	}
	return 0;
}

void Logger::Close()
{
	printf("%s(%d):<%s> log close pid=%d tid=%d\r\n", __FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self());
	if (m_server != NULL)
	{
		SocketBase* tmp = m_server;
		m_server = NULL;
		delete tmp;
	}
	m_epoll.Close();
	m_thread.Stop();
	if (m_file != NULL)
	{
		FILE* tmp = m_file;
		m_file = NULL;
		fclose(tmp);
	}
}

void Logger::Trace(const LogInfo& info)
{
	int ret = 0;
	static thread_local CSocket client;//每个线程对应一个对象，该对象的生命周期是整个线程存储期
	if (client == -1)
	{
		ret = client.Init(SockParam("./log/server.sock"));
		if (ret != 0)
		{
			client.Close();
			return;
		}
		ret = client.Link();
		if (ret != 0)
		{
			client.Close();
			return;
		}
	}
	//printf("%s(%d):<%s> pid=%d tid=%d client=%d\r\n", __FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), (int)client);
	ret = client.Send(info);
	//printf("%s(%d):<%s> pid=%d tid=%d client=%d ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), (int)client, ret);
	if (ret != 0)
	{
		client.Close();
		return;
	}
}

Buffer Logger::GetTimeStr()
{
	Buffer result(128);
	timeb tmb;
	ftime(&tmb);
	tm* pTm = localtime(&tmb.time);
	int nSize = snprintf(result, result.size(),
		"%04d-%02d-%02d_%02d-%02d-%02d_%03d",
		pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday,
		pTm->tm_hour, pTm->tm_min, pTm->tm_sec, tmb.millitm);
	result.resize(nSize);
	return result;
}

void Logger::WriteLog(const Buffer& data)
{
	if (m_file != NULL)
	{
		FILE* pFile = m_file;
		fwrite((char*)data, 1, data.size(), pFile);
		fflush(pFile);
#ifdef _Debug
		printf("%s(%d)<%s>:%s\r\n", __FILE__, __LINE__, __FUNCTION__, (char*)data);
#endif // _Debug
	}
}

LogInfo::LogInfo(const char* file, int line, const char* funcname, pid_t pid, pthread_t tid, LogLevel level, const char* fmt, ...)
{
	isAuto = false;
	const char* pLevel[5] = { "Information","Debug","Warning","Error","Fatal" };
	char* buf = NULL;
	int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%d> <%s>",
		file, line, pLevel[level], (char*)Logger::GetTimeStr(), pid, tid, funcname);
	if (count > 0)
	{
		m_buf += buf;
		free(buf);
	}
	else { return; }
	//------------------------------------
	//将额外参数添加到缓冲区中
	//------------------------------------
	va_list lst;
	va_start(lst, fmt);
	count = vasprintf(&buf, fmt, lst);
	if (count > 0)
	{
		m_buf += buf;
		free(buf);
	}
	else { return; }
	va_end(lst);
	m_buf += "\n";
}

LogInfo::LogInfo(const char* file, int line, const char* funcname, pid_t pid, pthread_t tid, LogLevel level)
{
	isAuto = true;
	const char* pLevel[5] = { "Information","Debug","Warning","Error","Fatal" };
	char* buf = NULL;
	int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%d> <%s>",
		file, line, pLevel[level], (char*)Logger::GetTimeStr(), pid, tid, funcname);
	if (count > 0)
	{
		m_buf += buf;
		free(buf);
	}
	else { return; }
}

LogInfo::LogInfo(const char* file, int line, const char* funcname, pid_t pid, pthread_t tid, LogLevel level, void* pData, size_t nSize)
{
	isAuto = false;
	const char* pLevel[5] = { "Information","Debug","Warning","Error","Fatal" };
	char* buf = NULL;
	int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%d> <%s>",
		file, line, pLevel[level], (char*)Logger::GetTimeStr(), pid, tid, funcname);
	if (count > 0)
	{
		m_buf += buf;
		free(buf);
	}
	else { return; }

	Buffer out;
	size_t i = 0;
	char* data = (char*)pData;
	m_buf += "\n";
	for (; i < nSize; i++)
	{
		char buf[16] = "";
		snprintf(buf, sizeof(buf), "%02X ", data[i] & 0xFF);
		m_buf += buf;
		if (0 == (i + 1) % 16)
		{
			m_buf += "\t | ";
			for (size_t j = i - 15; j <= i; j++)
			{
				if (((data[j] & 0xFF) > 31) && ((data[j] & 0xFF) < 0x7F))
				{
					m_buf += data[j];
				}
				else
				{
					m_buf += '.';
				}
			}
			m_buf += "\n";
		}
	}
	size_t k = i % 16;
	if (k != 0)
	{
		for (size_t j = 0; j < 16 - k; j++)m_buf += "   ";
		m_buf += "\t | ";
		for (size_t j = i - k; j <= i; j++)
		{
			if (((data[j] & 0xFF) > 31) && ((data[j] & 0xFF) < 0x7F))
			{
				m_buf += data[j];
			}
			else
			{
				m_buf += '.';
			}
		}
		m_buf += "\n";
	}
	m_buf += "\n";
}

LogInfo::~LogInfo()
{
	if (isAuto)
	{
		m_buf += "\n";
		Logger::Trace(*this);
	}
}

LogInfo::operator Buffer() const
{
	return m_buf;
}
