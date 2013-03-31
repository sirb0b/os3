#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/linkage.h>
#include <asm/uaccess.h>
#include <linux/unistd.h>
#include <asm/unistd.h>
//#include <asm/unistd_32.h>

asmlinkage int sys_set_colors(int nr_pids,pid_t *pids, u_int16_t *colors, int *retval){
  if(current_uid()!= 0){
    printk("<0>" "\ncurrent user not have enough permission\n");
    return -EACCES;
  }

  pid_t *pids_buffer;
  pids_buffer  =  kmalloc(nr_pids*sizeof(pid_t),GFP_KERNEL);
  u_int16_t *colors_buffer;
  colors_buffer  =  kmalloc(nr_pids*sizeof(u_int16_t),GFP_KERNEL);
  int *retval_buffer;
  retval_buffer  =  kmalloc(nr_pids*sizeof(int),GFP_KERNEL);

  printk("ENTER SYSTEMCALL");
  if (copy_from_user(pids_buffer, pids, nr_pids*sizeof(pid_t)))
      return -EFAULT;

  if (copy_from_user(colors_buffer, colors, nr_pids*sizeof(u_int16_t)))
    return -EFAULT;
  
  if (copy_from_user(retval_buffer, retval, nr_pids*sizeof(int)))
    return -EFAULT;

  pid_t *tgids_buffer;
  tgids_buffer = kmalloc(nr_pids*sizeof(pid_t),GFP_KERNEL);

  rcu_read_lock();
  int i = 0;
  for (i = 0; i<nr_pids; i++){
    struct task_struct *cur;

    pid_t pid = pids_buffer[i];
    u_int16_t color = colors_buffer[i];
    
    //read_lock(&tasklist_lock);
    if((cur = find_task_by_vpid(pid))!= NULL){
      cur->color = color;

      retval_buffer[i] = 0;
      tgids_buffer[i] = cur->tgid;
    }
    else{
      retval_buffer[i] = -EINVAL;
      tgids_buffer[i] = -1; //negtive value?
    }
      
    if(copy_to_user(retval,retval_buffer,nr_pids*sizeof(int)))
      return -EFAULT;
    
    printk("<0>" "\nThe cur task is %s [%d]\n",cur->comm, cur->pid);
    printk("<0>" "\nThe cur color is %d\n",cur->color);
  }
  
  //tranverse task
  struct task_struct *task;
  
  for_each_process(task){
    int i;
    for(i = 0; i<nr_pids; i++){
      if(task->tgid == tgids_buffer[i])
	task->color = colors_buffer[i];
    }
  }
  
  rcu_read_unlock();
//read_unlock(&tasklist_lock);
  
  kfree(pids_buffer);
  kfree(colors_buffer);
  kfree(retval_buffer);
  return 0;
}

asmlinkage int sys_get_colors(int nr_pids,pid_t *pids, u_int16_t *colors, int *retval){

  pid_t *pids_buffer;
  if((pids_buffer  =  kmalloc(nr_pids*sizeof(pid_t),GFP_KERNEL)) == 0)
  	return -1;
  u_int16_t *colors_buffer;
  if((colors_buffer  =  kmalloc(nr_pids*sizeof(u_int16_t),GFP_KERNEL)) == 0)
  	return -1;
  int *retval_buffer;
  if((retval_buffer  =  kmalloc(nr_pids*sizeof(int),GFP_KERNEL)) == 0)
  	return -1;

  if (copy_from_user(pids_buffer, pids, nr_pids*sizeof(pid_t)))
      return -EFAULT;

  int i = 0;
  
  for (i = 0;i<nr_pids;i++){
    pid_t pid = pids_buffer[i];
    struct task_struct *cur;
    if((cur = find_task_by_vpid(pid))!= NULL){
      colors_buffer[i] = cur->color;
      retval_buffer[i] = 0;
      printk("<0>" "\nThe cur task is %s [%d]\n",cur->comm, cur->pid);
      printk("<0>" "\nThe cur color is %d\n",cur->color);
    }
    
    else{
      colors_buffer[i] = 0;
      retval_buffer[i] = -EINVAL;
    }
    
    if(copy_to_user(colors,colors_buffer,nr_pids*sizeof(u_int16_t)))
      return -EFAULT;

    if(copy_to_user(retval,retval_buffer,nr_pids*sizeof(int)))
      return -EFAULT;
    
  }  

  kfree(pids_buffer);
  kfree(colors_buffer);
  kfree(retval_buffer);
  return 0;
}
