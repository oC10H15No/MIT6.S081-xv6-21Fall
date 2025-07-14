#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  backtrace();

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_sigalarm(void)
{
  int interval;
  uint64 handler_addr;
  struct proc *p = myproc();

  // Get the first argument: the interval in ticks.
  argint(0, &interval);
  // Get the second argument: the address of the handler function.
  argaddr(1, &handler_addr);

  // Store the interval and handler in the process structure.
  p->alarm_interval = interval;
  p->alarm_handler = (void (*)())handler_addr;
  p->ticks_passed = 0;

  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();

  // 恢复所有用户态寄存器，但不包括内核专属字段。
  // 我们从 epc 开始恢复，一直到最后一个寄存器。
  p->trapframe->epc = p->alarm_trackframe_backup.epc;
  p->trapframe->ra = p->alarm_trackframe_backup.ra;
  p->trapframe->sp = p->alarm_trackframe_backup.sp;
  p->trapframe->gp = p->alarm_trackframe_backup.gp;
  p->trapframe->tp = p->alarm_trackframe_backup.tp;
  p->trapframe->t0 = p->alarm_trackframe_backup.t0;
  p->trapframe->t1 = p->alarm_trackframe_backup.t1;
  p->trapframe->t2 = p->alarm_trackframe_backup.t2;
  p->trapframe->s0 = p->alarm_trackframe_backup.s0;
  p->trapframe->s1 = p->alarm_trackframe_backup.s1;
  p->trapframe->a0 = p->alarm_trackframe_backup.a0;
  p->trapframe->a1 = p->alarm_trackframe_backup.a1;
  p->trapframe->a2 = p->alarm_trackframe_backup.a2;
  p->trapframe->a3 = p->alarm_trackframe_backup.a3;
  p->trapframe->a4 = p->alarm_trackframe_backup.a4;
  p->trapframe->a5 = p->alarm_trackframe_backup.a5;
  p->trapframe->a6 = p->alarm_trackframe_backup.a6;
  p->trapframe->a7 = p->alarm_trackframe_backup.a7;
  p->trapframe->s2 = p->alarm_trackframe_backup.s2;
  p->trapframe->s3 = p->alarm_trackframe_backup.s3;
  p->trapframe->s4 = p->alarm_trackframe_backup.s4;
  p->trapframe->s5 = p->alarm_trackframe_backup.s5;
  p->trapframe->s6 = p->alarm_trackframe_backup.s6;
  p->trapframe->s7 = p->alarm_trackframe_backup.s7;
  p->trapframe->s8 = p->alarm_trackframe_backup.s8;
  p->trapframe->s9 = p->alarm_trackframe_backup.s9;
  p->trapframe->s10 = p->alarm_trackframe_backup.s10;
  p->trapframe->s11 = p->alarm_trackframe_backup.s11;
  p->trapframe->t3 = p->alarm_trackframe_backup.t3;
  p->trapframe->t4 = p->alarm_trackframe_backup.t4;
  p->trapframe->t5 = p->alarm_trackframe_backup.t5;
  p->trapframe->t6 = p->alarm_trackframe_backup.t6;

  p->handling_alarm = 0; // Reset the handling alarm flag

  return p->trapframe->a0; // Return the value in a0
}
