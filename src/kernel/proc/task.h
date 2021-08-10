#pragma once

#include <cpu/idt.h>
#include <include/list.h>

enum thread_state
{
	THREAD_NEW,
	THREAD_READY,
	THREAD_RUNNING,
	THREAD_WAITING,
	THREAD_TERMINATED,
} thread_state;

enum thread_policy
{
	THREAD_KERNEL_POLICY,
	THREAD_SYSTEM_POLICY,
	THREAD_APP_POLICY,
} thread_policy;

struct thread
{
	tid_t tid;
	uint32_t flags;
	enum thread_state state;
	enum thread_policy policy;
	int32_t priority;  // input priority, `sched_sibling.prio` is adjusted number based on scheduler
	struct process *parent;

	uint32_t esp;
	uint32_t kernel_stack;
	uint32_t user_stack;
	struct interrupt_registers uregs;

	sigset_t pending;
	sigset_t blocked;
	bool signaling;

	uint32_t time_slice;

	struct plist_node sched_sibling;
	struct timer_list sleep_timer;
};

struct process
{
	pid_t pid;
	gid_t gid;
	sid_t sid;

	char *name;
	struct process *parent;
	struct thread *thread;
	struct pdirectory *pdir;

	struct fs_struct *fs;
	struct files_struct *files;
	struct mm_struct *mm;

	struct tty_struct *tty;
	struct sigaction sighand[NSIG];
	int32_t exit_code;
	int32_t caused_signal;
	uint32_t flags;
	struct wait_queue_head wait_chld;

	struct list_head sibling;
	struct list_head children;
	struct timer_list sig_alarm_timer;
};

volatile struct thread *current_thread;
volatile struct process *current_process;