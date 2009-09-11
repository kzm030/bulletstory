#include"TaskSystem.h"

#define MEM_SIZE  10240		// �������̈�̃o�C�g��
#define DFRG_SIZE   256		// ���g�p�̈悪���̃o�C�g�������ɂȂ�����f�t���O

static BYTE *g_buf=NULL;		// �������̈�̐擪���w���|�C���^

BYTE* Task::m_active=NULL;		// �ŏ��Ɏ��s����^�X�N���w���|�C���^
BYTE* Task::m_free=NULL;		// �V�����^�X�N��ǉ����関�g�p�̈�̐擪���w���|�C���^

static DWORD g_size=0;		// �g�p���̃^�X�N�Ɋ��蓖�ĂĂ���o�C�g��
static DWORD g_count=0;		// �g�p���̃^�X�N��

// �^�X�N���X�g�̏�����
void Task::InitTaskList(void)
{
	ReleaseTaskList();

	g_buf=(BYTE*)malloc(MEM_SIZE);		// �������̈�̊m��
	memset(g_buf,0,MEM_SIZE);

	m_active=NULL;
	m_free=g_buf;

	g_size=0;
	g_count=0;
}

// �^�X�N���X�g�̉��
void Task::ReleaseTaskList(void)
{
	if(g_buf==NULL) return;		// �^�X�N���X�g������������Ă��Ȃ�

	for(Task *task=(Task*)m_active; m_active; task=(Task*)m_active)
	{
		delete task;
	}

	m_free=NULL;

	free(g_buf);		// �������̈�̉��
	g_buf=NULL;
}

// �����ȊO�̃^�X�N���폜
void Task::Delete(void)
{
	for(Task *task=this->m_next; task!=this; task=task->m_next)
	{
		delete task;
	}
}

// �S�Ẵ^�X�N�����s
void Task::RunTask(void)
{
	if(g_buf==NULL) return;		// �^�X�N���X�g������������Ă��Ȃ�

	Task *task,*next;
	for(task=(Task*)m_active; m_active; task=next)
	{
		task->Main();
		next=task->m_next;
		if(next==(Task*)m_active) break;
	}

	if(g_buf+MEM_SIZE-m_free < DFRG_SIZE) Defrag();		// �f�t���O
}

// �^�X�N���X�g�̃f�t���O
void Task::Defrag(void)
{
	if(m_active==NULL)		// �g�p���̃^�X�N������
	{
		m_free=g_buf;
		memset(g_buf,0,MEM_SIZE);
		return;
	}

	BYTE *dest=g_buf;

	Task *task;
	DWORD size;

	task=(Task*)m_active;
	if(task->m_next == task)		// �B��̃^�X�N���H
	{
		size=task->m_size;

		memmove(g_buf,m_active,size);

		task=(Task*)g_buf;
		task->m_pre  = task;
		task->m_next = task;

		m_active=g_buf;
		dest+=size;
	}
	else
	{
		for(BYTE *source=g_buf; source<m_free; source+=size)
		{
			task=(Task*)source;
			size=task->m_size;
			if(task->m_use == FALSE) continue;

			memmove(dest,source,size);

			task=(Task*)dest;
			task->m_pre->m_next = task;
			task->m_next->m_pre = task;

			if(source==m_active) m_active=dest;		// �ŏ��Ɏ��s����^�X�N���H
			dest+=size;
		}
	}

	memset(dest,0,m_free-dest);		// �V�������g�p�ɂȂ����̈�� 0 ����������

	m_free=dest;
}

// �^�X�N�̐����E�ǉ�
// �^�X�N�͗D��x�̒l�����������Ɍq����
void* Task::operator new(size_t size,float priority)
{
	if(g_buf==NULL) return NULL;		// �^�X�N���X�g������������Ă��Ȃ�
	if(m_free+size >= g_buf+MEM_SIZE) return NULL;		// �󂫗e�ʕs��

	g_size+=(DWORD)size;
	g_count++;

	Task *new_task=(Task*)m_free;
	m_free+=size;

	if(m_active==NULL)		// ���݃^�X�N���X�g�͋�
	{
		m_active=(BYTE*)new_task;

		new_task->m_use      = TRUE;
		new_task->m_size     = (DWORD)size;
		new_task->m_pre      = new_task;
		new_task->m_next     = new_task;
		new_task->m_priority = priority;

		return new_task;
	}

	// �^�X�N���X�g�ɑ}������

	Task *task,*next;
	for(task=(Task*)m_active;;task=next)
	{
		next=task->m_next;

		if(priority < task->m_priority)		// �����D��x�Ȃ疖���ɑ}��
		{
			if(task==(Task*)m_active)		// �擪�ɑ}��
			{
				m_active=(BYTE*)new_task;
			}
			new_task->m_pre  = task->m_pre;
			new_task->m_next = task;
			break;
		}
		else if(next==(Task*)m_active)		// �^�X�N���X�g�̖����ɑ}��
		{
			new_task->m_pre  = task;
			new_task->m_next = next;
			break;
		}
	}

	new_task->m_use      = TRUE;
	new_task->m_size     = (DWORD)size;
	new_task->m_priority = priority;

	new_task->m_pre->m_next = new_task;
	new_task->m_next->m_pre = new_task;

	return new_task;
}

// �^�X�N�̍폜
void Task::operator delete(void *pTask)
{
	if(pTask==NULL) return;
	Task *task=(Task*)pTask;

	if(task->m_use==FALSE) return;		// �d���폜�h�~
	task->m_use=FALSE;		// ���̃����o�ϐ��͌�Ŏg�����ߏ����Ȃ�

	g_size -= task->m_size;
	g_count--;

	if(task==(Task*)m_active)		// �ŏ��Ɏ��s����^�X�N���H
	{
		if(task->m_next==(Task*)m_active)		// �Ō�̃^�X�N���H
		{
			m_active=NULL;
			return;
		}
		else m_active=(BYTE*)task->m_next;
	}

	task->m_pre->m_next = task->m_next;
	task->m_next->m_pre = task->m_pre;
}

// �D��x�̕ύX �� �^�X�N���X�g�̌q������
// �^�X�N�͗D��x�̒l�����������Ɍq����
void Task::SetPriority(float priority)
{
	m_priority=priority;

	// �^�X�N���X�g���珜�O����

	if(this==(Task*)m_active)		// �ŏ��Ɏ��s����^�X�N���H
	{
		if(m_next==(Task*)m_active) return;		// �B��̃^�X�N���H
		m_active=(BYTE*)m_next;
	}

	m_pre->m_next = m_next;
	m_next->m_pre = m_pre;

	// �^�X�N���X�g�ɑ}������

	Task *active=(Task*)m_active;

	Task *task,*next;
	for(task=active;;task=next)
	{
		next=task->m_next;

		if(priority < task->m_priority)		// �����D��x�Ȃ疖���ɑ}��
		{
			if(task==(Task*)m_active)		// �擪�ɑ}��
			{
				m_active=(BYTE*)this;
			}
			m_pre  = task->m_pre;
			m_next = task;
			break;
		}
		else if(next==active)		// �^�X�N���X�g�̖����ɑ}��
		{
			m_pre  = task;
			m_next = next;
			break;
		}
	}

	m_pre->m_next = this;
	m_next->m_pre = this;

	// �ŏ��Ɏ��s����^�X�N���w���|�C���^���Đݒ�
	if(priority < active->m_priority) m_active=(BYTE*)this;
}

// �g�p���̃^�X�N�Ɋ��蓖�ĂĂ���o�C�g����Ԃ�
DWORD Task::GetSize(void)
{
	return g_size;
}

// �g�p���̃^�X�N����Ԃ�
DWORD Task::GetCount(void)
{
	return g_count;
}

// �������̈�̓��e���e�L�X�g�t�@�C���ɏ����o��
void Task::Dump(const char *filename)
{
	if(g_buf==NULL) return;		// �^�X�N���X�g������������Ă��Ȃ�

	FILE *fp=fopen(filename,"w");

	int i;

	fprintf(fp,"          ");
	for(i=0;i<16;i++)
	{
		if(i%16 == 8) fprintf(fp," ");
		fprintf(fp," %02x",i);
	}
	fprintf(fp,"\n");

	fprintf(fp,"          ");
	for(i=0;i<16;i++)
	{
		if(i%16 == 8) fprintf(fp,"-");
		fprintf(fp,"---");
	}
	fprintf(fp,"\n");

	for(i=0;i<MEM_SIZE;i++)
	{
		if(i%16 == 0) fprintf(fp,"%p |",g_buf+i);
		else if(i%16 == 8) fprintf(fp," ");
		fprintf(fp," %02x",*(g_buf+i));
		if(i%16 == 15) fprintf(fp,"\n");
	}

	fclose(fp);
}
