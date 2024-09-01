#pragma once
#include <unistd.h>
#include <sys/epoll.h>
#include <vector>
#include <errno.h>
#include <memory.h>
#define EPOLL_SIZE 128

class EPdata
{
public:
	EPdata() { m_data.u64 = 0; }
	EPdata(void* ptr) { m_data.ptr = ptr; }
	explicit EPdata(int& p) { m_data.fd = p; }
	explicit EPdata(uint32_t& p) { m_data.u32 = p; }
	explicit EPdata(uint64_t& p) { m_data.u64 = p; }
	EPdata(const EPdata& data) { m_data.u64 = data.m_data.u64; }
	~EPdata() {}
public:
	EPdata& operator=(const EPdata& data) { m_data.u64 = data.m_data.u64; return *this; }
	EPdata& operator=(void* ptr) { m_data.ptr = ptr; return *this; }
	EPdata& operator=(int& p) { m_data.fd = p; return *this; }
	EPdata& operator=(uint32_t& p) { m_data.u32 = p; return *this; }
	EPdata& operator=(uint64_t& p) { m_data.u64 = p; return *this; }
	operator epoll_data_t() { return m_data; }
	operator epoll_data_t()const { return m_data; }
	operator epoll_data_t* () { return &m_data; }
	operator const epoll_data_t* ()const { return &m_data; }
private:
	epoll_data_t m_data;
};

using EPevents = std::vector<epoll_event>;

class CEpoll
{
public:
	CEpoll();
	~CEpoll();
public:
	//CEpoll是内核的对象，无法复制
	CEpoll(const CEpoll&) = delete;
	CEpoll& operator=(const CEpoll&) = delete;
public:
	operator int()const { return m_epoll; }
public:
	int Create(int size);
	ssize_t WaitForEpoll(EPevents& events, int timeout = 10);
	int Add(int fd, const EPdata& data = EPdata(), uint32_t events = EPOLLIN|EPOLLERR);
	int Mod(int fd, uint32_t events, const EPdata& data = EPdata());
	int Del(int fd);
	void Close();
private:
	int m_epoll;
};

