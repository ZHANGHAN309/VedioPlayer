#pragma once
#include <unistd.h>//包含一些数据类型和api函数
#include <functional>//与模板有关

//模板函数基类 可以通过基类指针访问模板函数
class FuncBase
{
public:
	FuncBase() {}
	virtual ~FuncBase() {}//使用虚析构保证子类可以被析构
	virtual int operator()() = 0;
};

//模板函数类 
//通过传入类型生成函数，传入参数数量可变
//传入的第一个参数为函数指针，其余参数为函数指针的参数
//比如进程入口函数 线程入口函数
template<typename _FUNC_, typename... _ARG_>
class CFunction :public FuncBase
{
public:
	CFunction(_FUNC_ func, _ARG_... args)
		:m_binder(std::forward<_FUNC_>(func), std::forward<_ARG_>(args)...)//std::forward表示原样转发
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