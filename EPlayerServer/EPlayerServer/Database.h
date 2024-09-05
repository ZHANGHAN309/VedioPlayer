#pragma once
#include "Public.h"
#include <map>
#include <list>
#include <vector>
#include <memory>

class _Table_;
using PTable = std::shared_ptr<_Table_>;
using KeyValue = std::map<Buffer, Buffer>;
using Result = std::list<PTable>;

class DatabaseClient
{
public:
	DatabaseClient(const DatabaseClient&) = delete;
	DatabaseClient& operator=(const DatabaseClient&) = delete;
public:
	DatabaseClient() {}
	virtual ~DatabaseClient() {}
	//��������
	virtual int StartTransaction() = 0;
	//�ύ����
	virtual int CommitTransaction() = 0;
	//�ع�����
	virtual int RollbackTransaction() = 0;
	//����
	virtual int Connect(const KeyValue& args) = 0;
	//ִ��
	virtual int Exec(const Buffer& sql) = 0;
	//�������ִ��
	virtual int Exec(const Buffer& sql, Result& result, const _Table_& table) = 0;
	//�ر�����
	virtual int Close() = 0;
	//�Ƿ�����
	virtual bool IsConnected() = 0;
};

class _Field_;
using PField = std::shared_ptr<_Field_>;
using FieldArray = std::vector<PField>;
using FieldMap = std::map<Buffer, PField>;

class _Table_
{
public:
	_Table_() {}
	virtual ~_Table_() {}
	//���ش�����sql���
	virtual Buffer Create() = 0;
	//ɾ����
	virtual Buffer Drop() = 0;
	//��ɾ�Ĳ�
	virtual Buffer Insert(const _Table_& values) = 0;
	virtual Buffer Delete(const _Table_& values) = 0;
	virtual Buffer Modify(const _Table_& values) = 0;
	virtual Buffer Query(const Buffer& condition = "") = 0;
	//����һ��_Table_����
	virtual PTable Copy()const = 0;
	virtual void ClearFieldUsed() = 0;
public:
	//��ȡ���ȫ��
	virtual operator const Buffer()const = 0;
public:
	//��������DB����
	Buffer Database;
	//�������
	Buffer Name;
	//�еĶ���
	FieldArray FieldDefine;
	//�еĶ���ӳ���
	FieldMap Fields;
};

enum {
	SQL_INSERT = 1,//�������
	SQL_MODIFY = 2,//�޸ĵ���
	SQL_CONDITION = 4//��ѯ������
};

enum {
	NONE = 0,
	NOT_NULL = 1,
	DEFAULT = 2,
	UNIQUE = 4,
	PRIMARY_KEY = 8,
	CHECK = 16,
	AUTOINCREMENT = 32
};

using SqlType = enum {
	TYPE_NULL = 0,
	TYPE_BOOL = 1,
	TYPE_INT = 2,
	TYPE_DATETIME = 4,
	TYPE_REAL = 8,
	TYPE_VARCHAR = 16,
	TYPE_TEXT = 32,
	TYPE_BLOB = 64
};

class _Field_
{
public:
	_Field_() {}
	_Field_(const _Field_& field)
	{
		Name = field.Name;
		Type = field.Type;
		Size = field.Size;
		Attr = field.Attr;
		Default = field.Default;
		Check = field.Check;
	}
	virtual ~_Field_() {}
	virtual _Field_& operator=(const _Field_& field)
	{
		if (this != &field)
		{
			Name = field.Name;
			Type = field.Type;
			Size = field.Size;
			Attr = field.Attr;
			Default = field.Default;
			Check = field.Check;
		}
		return *this;
	}
public:
	virtual Buffer Create() = 0;
	virtual void LoadFromStr(const Buffer& str) = 0;
	//where ���ʹ��
	virtual Buffer toEqualExp() const = 0;
	virtual Buffer toSqlStr() const = 0;
	//�е�ȫ��
	virtual operator const Buffer() const = 0;
public:
	Buffer Name;
	Buffer Type;
	Buffer Size;
	unsigned Attr;
	Buffer Default;
	Buffer Check;
	union {
		bool Bool;
		int Integer;
		double Double;
		Buffer* String;
	}Value;
	int nType;
	//��������
	unsigned Condition;
};

#define DECLARE_TABLE_CLASS(name, base) class name:public base { \
public: \
virtual PTable Copy() const {return PTable(new name(*this));} \
name():base(){Name=#name;

#define DECLARE_FIELD(ntype,name,attr,type,size,default_,check) \
{PField field(new _sqlite3_field_(ntype, #name, attr, type, size, default_, check));FieldDefine.push_back(field);Fields[#name] = field; }

#define DECLARE_TABLE_CLASS_EDN() }};