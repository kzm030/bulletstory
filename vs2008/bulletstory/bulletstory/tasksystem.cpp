#include"TaskSystem.h"

#define MEM_SIZE  10240		// メモリ領域のバイト数
#define DFRG_SIZE   256		// 未使用領域がこのバイト数未満になったらデフラグ

static BYTE *g_buf=NULL;		// メモリ領域の先頭を指すポインタ

BYTE* Task::m_active=NULL;		// 最初に実行するタスクを指すポインタ
BYTE* Task::m_free=NULL;		// 新しいタスクを追加する未使用領域の先頭を指すポインタ

static DWORD g_size=0;		// 使用中のタスクに割り当てているバイト数
static DWORD g_count=0;		// 使用中のタスク数

// タスクリストの初期化
void Task::InitTaskList(void)
{
	ReleaseTaskList();

	g_buf=(BYTE*)malloc(MEM_SIZE);		// メモリ領域の確保
	memset(g_buf,0,MEM_SIZE);

	m_active=NULL;
	m_free=g_buf;

	g_size=0;
	g_count=0;
}

// タスクリストの解放
void Task::ReleaseTaskList(void)
{
	if(g_buf==NULL) return;		// タスクリストが初期化されていない

	for(Task *task=(Task*)m_active; m_active; task=(Task*)m_active)
	{
		delete task;
	}

	m_free=NULL;

	free(g_buf);		// メモリ領域の解放
	g_buf=NULL;
}

// 自分以外のタスクを削除
void Task::Delete(void)
{
	for(Task *task=this->m_next; task!=this; task=task->m_next)
	{
		delete task;
	}
}

// 全てのタスクを実行
void Task::RunTask(void)
{
	if(g_buf==NULL) return;		// タスクリストが初期化されていない

	Task *task,*next;
	for(task=(Task*)m_active; m_active; task=next)
	{
		task->Main();
		next=task->m_next;
		if(next==(Task*)m_active) break;
	}

	if(g_buf+MEM_SIZE-m_free < DFRG_SIZE) Defrag();		// デフラグ
}

// タスクリストのデフラグ
void Task::Defrag(void)
{
	if(m_active==NULL)		// 使用中のタスクが無い
	{
		m_free=g_buf;
		memset(g_buf,0,MEM_SIZE);
		return;
	}

	BYTE *dest=g_buf;

	Task *task;
	DWORD size;

	task=(Task*)m_active;
	if(task->m_next == task)		// 唯一のタスクか？
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

			if(source==m_active) m_active=dest;		// 最初に実行するタスクか？
			dest+=size;
		}
	}

	memset(dest,0,m_free-dest);		// 新しく未使用になった領域に 0 を書き込む

	m_free=dest;
}

// タスクの生成・追加
// タスクは優先度の値が小さい順に繋げる
void* Task::operator new(size_t size,float priority)
{
	if(g_buf==NULL) return NULL;		// タスクリストが初期化されていない
	if(m_free+size >= g_buf+MEM_SIZE) return NULL;		// 空き容量不足

	g_size+=(DWORD)size;
	g_count++;

	Task *new_task=(Task*)m_free;
	m_free+=size;

	if(m_active==NULL)		// 現在タスクリストは空
	{
		m_active=(BYTE*)new_task;

		new_task->m_use      = TRUE;
		new_task->m_size     = (DWORD)size;
		new_task->m_pre      = new_task;
		new_task->m_next     = new_task;
		new_task->m_priority = priority;

		return new_task;
	}

	// タスクリストに挿入する

	Task *task,*next;
	for(task=(Task*)m_active;;task=next)
	{
		next=task->m_next;

		if(priority < task->m_priority)		// 同じ優先度なら末尾に挿入
		{
			if(task==(Task*)m_active)		// 先頭に挿入
			{
				m_active=(BYTE*)new_task;
			}
			new_task->m_pre  = task->m_pre;
			new_task->m_next = task;
			break;
		}
		else if(next==(Task*)m_active)		// タスクリストの末尾に挿入
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

// タスクの削除
void Task::operator delete(void *pTask)
{
	if(pTask==NULL) return;
	Task *task=(Task*)pTask;

	if(task->m_use==FALSE) return;		// 重複削除防止
	task->m_use=FALSE;		// 他のメンバ変数は後で使うため消さない

	g_size -= task->m_size;
	g_count--;

	if(task==(Task*)m_active)		// 最初に実行するタスクか？
	{
		if(task->m_next==(Task*)m_active)		// 最後のタスクか？
		{
			m_active=NULL;
			return;
		}
		else m_active=(BYTE*)task->m_next;
	}

	task->m_pre->m_next = task->m_next;
	task->m_next->m_pre = task->m_pre;
}

// 優先度の変更 と タスクリストの繋ぎ直し
// タスクは優先度の値が小さい順に繋げる
void Task::SetPriority(float priority)
{
	m_priority=priority;

	// タスクリストから除外する

	if(this==(Task*)m_active)		// 最初に実行するタスクか？
	{
		if(m_next==(Task*)m_active) return;		// 唯一のタスクか？
		m_active=(BYTE*)m_next;
	}

	m_pre->m_next = m_next;
	m_next->m_pre = m_pre;

	// タスクリストに挿入する

	Task *active=(Task*)m_active;

	Task *task,*next;
	for(task=active;;task=next)
	{
		next=task->m_next;

		if(priority < task->m_priority)		// 同じ優先度なら末尾に挿入
		{
			if(task==(Task*)m_active)		// 先頭に挿入
			{
				m_active=(BYTE*)this;
			}
			m_pre  = task->m_pre;
			m_next = task;
			break;
		}
		else if(next==active)		// タスクリストの末尾に挿入
		{
			m_pre  = task;
			m_next = next;
			break;
		}
	}

	m_pre->m_next = this;
	m_next->m_pre = this;

	// 最初に実行するタスクを指すポインタを再設定
	if(priority < active->m_priority) m_active=(BYTE*)this;
}

// 使用中のタスクに割り当てているバイト数を返す
DWORD Task::GetSize(void)
{
	return g_size;
}

// 使用中のタスク数を返す
DWORD Task::GetCount(void)
{
	return g_count;
}

// メモリ領域の内容をテキストファイルに書き出す
void Task::Dump(const char *filename)
{
	if(g_buf==NULL) return;		// タスクリストが初期化されていない

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
