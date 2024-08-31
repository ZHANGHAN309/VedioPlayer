#pragma once
#include <unistd.h>//����һЩ�������ͺ�api����
#include <functional>//��ģ���й�

//ģ�庯������ ����ͨ������ָ�����ģ�庯��
class FuncBase
{
public:
	FuncBase() {}
	virtual ~FuncBase() {}//ʹ����������֤������Ա�����
	virtual int operator()() = 0;
};

//ģ�庯���� 
//ͨ�������������ɺ�����������������ɱ�
//����ĵ�һ������Ϊ����ָ�룬�������Ϊ����ָ��Ĳ���
//���������ں��� �߳���ں���
template<typename _FUNC_, typename... _ARG_>
class CFunction :public FuncBase
{
public:
	CFunction(_FUNC_ func, _ARG_... args)
		:m_binder(std::forward<_FUNC_>(func), std::forward<_ARG_>(args)...)//std::forward��ʾԭ��ת��
	{
	}
	virtual ~CFunction() {}
	virtual int operator()()
	{
		return m_binder();
	}
private:
	typename std::_Bindres_helper<int, _FUNC_, _ARG_...>::type m_binder;
};