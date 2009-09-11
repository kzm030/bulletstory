#pragma once

#pragma warning(disable:4291)		// new ���Z�q�̌x���𖳌��ɂ���

#include<stdio.h>
#include<malloc.h>		// for malloc, free
#include<string.h>		// for memset, memmove

typedef unsigned char BYTE;
typedef unsigned long DWORD;

#define TRUE  1
#define FALSE 0

class Task
{
	BYTE m_use;		// TRUE �g�p�� / FALSE ��g�p
	DWORD m_size;		// �o�C�g��
	float m_priority;		// �D��x
	static void Defrag(void);
protected:
	static BYTE *m_active,*m_free;
	Task *m_pre,*m_next;		// �O�Ǝ��̃^�X�N���w���|�C���^
public:
	virtual ~Task(void){}		// ���z�f�X�g���N�^

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

	virtual void Main(void)=0;		// �����֐�
};
