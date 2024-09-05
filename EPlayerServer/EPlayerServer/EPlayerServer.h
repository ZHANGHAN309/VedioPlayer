#pragma once
#include "CServer.h"
#include "jsoncpp/json.h"
#include "HttpParser.h"
#include "Crypto.h"
#include "MysqlClient.h"
#include <map>

DECLARE_TABLE_CLASS(edoyunLogin_user_mysql, _mysql_table_)
DECLARE_MYSQL_FIELD(TYPE_INT, user_id, NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, "INTEGER", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_VARCHAR, user_qq, NOT_NULL, "VARCHAR", "(15)", "", "")  //QQ号
DECLARE_MYSQL_FIELD(TYPE_VARCHAR, user_phone, DEFAULT, "VARCHAR", "(11)", "'18888888888'", "")  //手机
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_name, NOT_NULL, "TEXT", "", "", "")    //姓名
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_nick, NOT_NULL, "TEXT", "", "", "")    //昵称
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_wechat, DEFAULT, "TEXT", "", "NULL", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_wechat_id, DEFAULT, "TEXT", "", "NULL", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_address, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_province, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_country, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_age, DEFAULT | CHECK, "INTEGER", "", "18", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_male, DEFAULT, "BOOL", "", "1", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_flags, DEFAULT, "TEXT", "", "0", "")
DECLARE_MYSQL_FIELD(TYPE_REAL, user_experience, DEFAULT, "REAL", "", "0.0", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_level, DEFAULT | CHECK, "INTEGER", "", "0", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_class_priority, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_REAL, user_time_per_viewer, DEFAULT, "REAL", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_career, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_password, NOT_NULL, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_birthday, NONE, "DATETIME", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_describe, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_education, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_register_time, DEFAULT, "DATETIME", "", "LOCALTIME()", "")
DECLARE_TABLE_CLASS_EDN()

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
		if (m_db != NULL)
		{
			DatabaseClient* tmp = m_db;
			m_db = NULL;
			tmp->Close();
			delete tmp;
		}
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
		m_db = new CMysqlClient();
		if (m_db == NULL)
		{
			TRACEE("no more memory!\r\n");
			return -1;
		}
		KeyValue args;
		args["host"] = "192.168.179.128";
		args["user"] = "root";
		args["password"] = "123456";
		args["port"] = 3306;
		args["db"] = "edoyun";
		ret = m_db->Connect(args);
		if (ret != 0)
		{
			TRACEE("database connect failed! ret=%d\r\n", ret);
			return -2;
		}
		edoyunLogin_user_mysql user;
		ret = m_db->Exec(user.Create());
		ERR_RETURN(ret, -3);
		ret = setConnected(&EPlayerServer::Connected, this, _1);
		ERR_RETURN(ret, -4);
		ret = setRecv(&EPlayerServer::Received, this, _1, _2);
		ERR_RETURN(ret, -5);
		ret = m_epoll.Create(m_count);
		ERR_RETURN(ret, -6);
		ret = m_pool.Start(m_count);
		ERR_RETURN(ret, -7);
		for (unsigned i = 0; i < m_count; i++)
		{
			ret = m_pool.AddTask(&EPlayerServer::ThreadFunc, this);
			ERR_RETURN(ret, -8);
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
				(*m_connected)(pClient);//需要传递什么参数
			}
		}
		return 0;
	}
	Buffer MakeResponse(int ret) {
		Json::Value root;
		root["status"] = ret;
		if (ret != 0) {
			root["message"] = "登录失败，可能是用户名或者密码错误！";
		}
		else {
			root["message"] = "success";
		}
		Buffer json = root.toStyledString();
		Buffer result = "HTTP/1.1 200 OK\r\n";
		time_t t;
		time(&t);
		tm* ptm = localtime(&t);
		char temp[64] = "";
		strftime(temp, sizeof(temp), "%a, %d %b %G %T GMT\r\n", ptm);
		Buffer Date = Buffer("Date: ") + temp;
		Buffer Server = "Server: Edoyun/1.0\r\nContent-Type: text/html; charset=utf-8\r\nX-Frame-Options: DENY\r\n";
		snprintf(temp, sizeof(temp), "%d", json.size());
		Buffer Length = Buffer("Content-Length: ") + temp + "\r\n";
		Buffer Stub = "X-Content-Type-Options: nosniff\r\nReferrer-Policy: same-origin\r\n\r\n";
		result += Date + Server + Length + Stub + json;
		TRACEI("response: %s", (char*)result);
		return result;
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
						TRACEI("pClient=%d Recv data:\r\n%s\r\n", (int)*pClient, data);
						WARNING_CONTINUE(ret);
						if (m_recv)
						{
							(*m_recv)(pClient, data);
						}
					}
				}
			}
		}
		return 0;
	}
	int Connected(SocketBase* pClient)
	{
		//客户端连接处理，简单打印一下客户端信息就行
		sockaddr_in* addr = *pClient;
		TRACEI("client connected addr %s port[%d]\r\n", inet_ntoa(addr->sin_addr), addr->sin_port);
		return 0;
	}
	int Received(SocketBase* pClient, const Buffer& data)
	{
		//主要业务
		int ret = 0;
		//HTTP解析
		Buffer response = "";
		ret = HttpParser(data);
		//验证结果的反馈
		if (ret != 0) {//验证失败
			TRACEE("http parser failed!%d", ret);
		}
		response = MakeResponse(ret);
		ret = pClient->Send(response);
		if (ret != 0) {
			TRACEE("http response failed!%d [%s]", ret, (char*)response);
		}
		else {
			TRACEI("http response success!%d", ret);
		}
		return 0;
	}
	int HttpParser(const Buffer& data)
	{
		CHttpParser parser;
		size_t size = parser.Parser(data);
		
		if (size == 0 || (parser.Errno() != 0))//解析失败
		{
			TRACEE("parser size=%lu http errno=%u\r\n", size, parser.Errno());
			return -1;
		}
		if (parser.Method() == HTTP_GET)
		{
			//get处理
			UrlParser url("https://192.168.179.128" + parser.Url());
			int ret = url.Parser();
			if (ret != 0)
			{
				TRACEE("url parser failed! ret=%d url=[%s]\r\n", ret, url);
				return -2;
			}
			Buffer uri = url.Uri();
			if (uri == "login")
			{
				//登录处理
				Buffer time = url["time"];
				Buffer salt = url["salt"];
				Buffer user = url["user"];
				Buffer sign = url["sign"];
				TRACEI("time=%s,salt=%s,user=%s,sign==%s\r\n", time, salt, user, sign);
				//数据库查询
				edoyunLogin_user_mysql dbuser;
				Result result;
				Buffer sql = dbuser.Query("user_name=\"" + user + "\"");
				ret = m_db->Exec(sql, result, dbuser);
				if (ret != 0) {
					TRACEE("sql=%s ret=%d", (char*)sql, ret);
					return -3;
				}
				if (result.size() == 0) {
					TRACEE("no result sql=%s ret=%d", (char*)sql, ret);
					return -4;
				}
				if (result.size() != 1) {
					TRACEE("more than one sql=%s ret=%d", (char*)sql, ret);
					return -5;
				}
				auto user1 = result.front();
				Buffer pwd = *user1->Fields["user_password"]->Value.String;
				TRACEI("password = %s", (char*)pwd);
				//登录请求的验证
				const char* MD5_KEY = "*&^%$#@b.v+h-b*g/h@n!h#n$d^ssx,.kl<kl";
				Buffer md5str = time + MD5_KEY + pwd + salt;
				Buffer md5 = Crypto::MD5(md5str);
				TRACEI("md5 = %s", (char*)md5);
				if (md5 == sign) {
					return 0;
				}
				return -6;
			}
		}
		else if (parser.Method() == HTTP_POST)
		{
			//post处理

		}
	}
private:
	CEpoll m_epoll;
	std::map<int, SocketBase*> m_mapClients;
	ThreadPool m_pool;
	unsigned m_count;
	DatabaseClient* m_db;
};