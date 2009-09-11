#pragma once

#pragma warning(disable:4291)		// new 演算子の警告を無効にする

#include<stdio.h>
#include<malloc.h>		// for malloc, free
#include<string.h>		// for memset, memmove

typedef unsigned char BYTE;
typedef unsigned long DWORD;

#define TRUE  1
#define FALSE 0

class Task
{
	BYTE m_use;		// TRUE 使用中 / FALSE 非使用
	DWORD m_size;		// バイト数
	float m_priority;		// 優先度
	static void Defrag(void);
protected:
	static BYTE *m_active,*m_free;
	Task *m_pre,*m_next;		// 前と次のタスクを指すポインタ
public:
	virtual ~Task(void){}		// 仮想デストラクタ

	static void InitTaskList(void);
	static void ReleaseTaskList(void);

	void Delete(void);
	static void RunTask(void);

	void* operator new(size_t size,float priority=0.5f);
	void operator delete(void *pTask);

	void SetPriority(float priority);

	static DWORD GetSize(void);
	static DWORD GetCount(void);

	static void Dump(const char *filename);

	virtual void Main(void)=0;		// 処理関数
};
