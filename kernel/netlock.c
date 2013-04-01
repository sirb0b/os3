#include <linux/init.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/linkage.h>
#include <asm/uaccess.h>
#include <linux/rwsem.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>

/*SLEEP is the Writer Lock
  USE is the reader lock*/
//declare a global wait queue to store the tasks
DECLARE_WAIT_QUEUE_HEAD(waitq); 

/*Get the event out*/
//test function
void my_timer_function(unsigned long data){
	wake_up_interruptible(&waitq);
	printk("<1>testtest2");
}

//use ATOMIC to simulate a RW lock
atomic_t reader=ATOMIC_INIT(0);
atomic_t writer=ATOMIC_INIT(0);
//alter the flag to wake up
int flag=0;
//store the number of processes in wait queue
atomic_t counter=ATOMIC_INIT(0);
unsigned long eD;

asmlinkage int sys_net_lock(netlock_t type, u_int16_t timeout_val){
	
	if(type == NET_LOCK_USE){
	    printk("<1> REQUIRE USE LOCK\n");
	    printk("<1>" "TEST PRINT\n");
            int wri=atomic_read(&writer);
            if(wri!=0){
                flag=0;
            	int nr=atomic_read(&counter);
            	if(nr==0) eD=jiffies+timeout_val*HZ;
            	current->absolute_deadline=eD;
            	atomic_inc(&counter);
            	if(jiffies+timeout_val*HZ<eD){
            		eD=jiffies+timeout_val*HZ;
            		printk("eD UPDATED-> %d\n",eD);
            	}
            	current->oldpolicy=current->policy;
            	current->policy=SCHED_EDF;
                wait_event_interruptible(waitq,flag);//current->lockflag);
            }
            else{
	        atomic_inc(&reader);
                current->nlocktype=NET_LOCK_USE;
            }
            return 11;
	}
	else if(type == NET_LOCK_SLEEP){
	    printk("<1> REQUIRE SLEEP LOCK\n");
	    printk("<1>CheckFlag %d\n",flag);
	    int wri=atomic_read(&writer);
            int rea=atomic_read(&reader);
            printk("<1> Val=> Writer %d  Reader %d\n", wri,rea);
            if(wri==0 && rea==0){
            	printk("<1>alter flag\n");
            	flag=0;
                atomic_inc(&writer);
                current->nlocktype=NET_LOCK_SLEEP;
                return 13;
            }   
        }
	printk("<1>net_lock Test Message");
	return 101;
}
struct timer_list my_timer;
asmlinkage int sys_net_unlock(){
    netlock_t ty=current->nlocktype;
    if(ty==NET_LOCK_SLEEP){
        atomic_dec(&writer);
        printk("<1>TIMEUP\n");
        return 1;
    }
    else if(ty==NET_LOCK_USE){
        atomic_dec(&reader);
        current->policy=current->oldpolicy;
        return 1;
    }
    return -1;
}

void my_timer_wakeup(){
	printk("<1>GET INTO TIMER\n");
	int cto=atomic_read(&writer);
	if(cto>0)
       			 atomic_dec(&writer);
        printk("<1>TIMEUPWAKEUP\n");
        int ct=atomic_read(&counter);
	atomic_add(ct,&reader);
        flag=1;
    	wake_up_all(&waitq);
    	atomic_set(&counter,0);
    	eD=INT_MAX;
}
struct timer_list releaseTimer;
asmlinkage int sys_net_lock_wait_timeout(){
	int ct=atomic_read(&counter);
	printk("<1>Number of ct: %d", ct);
	if(ct==0)
            	return -1;//which means there is no blocked process
        else{
		int ret;
		setup_timer(&releaseTimer, my_timer_wakeup, 0 );
		ret = mod_timer(&releaseTimer, eD);
		printk("<1>TimerStarted");
		printk("elapse-> %lu ->%lu-> %lu",eD-jiffies, eD, jiffies);
        }
	return 1;
}	

  
