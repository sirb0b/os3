//static const struct sched_class edf_sched_class;

struct edf_task * find_edf_task_list(struct edf_rq *rq, struct task_struct *p)
{
  struct list_head *ptr=NULL;
  struct edf_task *edf_task=NULL;
  if(rq && p){
    list_for_each(ptr,&rq->edf_list_head){
      edf_task=list_entry(ptr,struct edf_task, edf_list_node);
      if(edf_task){
	if(edf_task->task->edf_id==p->edf_id){
	  return edf_task;
	}
      }
    }
  }
  return NULL;
}

void insert_edf_task_rb_tree(struct edf_rq *rq, struct edf_task *p)
{
	struct rb_node **node=NULL;
	struct rb_node *parent=NULL;
	struct edf_task *entry=NULL;
	node=&rq->edf_rb_root.rb_node;
	while(*node!=NULL){
		parent=*node;
		entry=rb_entry(parent, struct edf_task,edf_rb_node);
		if(entry){
			if(p->absolute_deadline < entry->absolute_deadline){
				node=&parent->rb_left;
			}else{
				node=&parent->rb_right;
			}
		}
	}
	rb_link_node(&p->edf_rb_node,parent,node);
	rb_insert_color(&p->edf_rb_node,&rq->edf_rb_root);
}

void remove_edf_task_rb_tree(struct edf_rq *rq, struct edf_task *p)
{
	rb_erase(&(p->edf_rb_node),&(rq->edf_rb_root));
	p->edf_rb_node.rb_left=p->edf_rb_node.rb_right=NULL;
}

struct edf_task * earliest_deadline_edf_task_rb_tree(struct edf_rq *rq)
{
	struct rb_node *node=NULL;
	struct edf_task *p=NULL;
	node=rq->edf_rb_root.rb_node;
	if(node==NULL)
		return NULL;
	
	while(node->rb_left!=NULL){
		node=node->rb_left;
	}
	p=rb_entry(node, struct edf_task,edf_rb_node);
	return p;
}

void add_edf_task_2_list(struct edf_rq *rq, struct task_struct *p)
{
	struct list_head *ptr=NULL;
	struct edf_task *new=NULL, *edf_task=NULL;
	if(rq && p){
		new=(struct edf_task *) kzalloc(sizeof(struct edf_task),GFP_KERNEL);
		if(new){
			edf_task=NULL;
			new->task=p;
			new->absolute_deadline=0;
			list_for_each(ptr,&rq->edf_list_head){
				edf_task=list_entry(ptr,struct edf_task, edf_list_node);
				if(edf_task){
					if(edf_task->task->edf_id < edf_task->task->edf_id){
						list_add(&new->edf_list_node,ptr);
					}
				}
			}
			list_add(&new->edf_list_node,&rq->edf_list_head);
		       	}
		else{
			printk(KERN_ALERT "add_edf_task_2_list: kzalloc\n");
		}
	}
	else{
		printk(KERN_ALERT "add_edf_task_2_list: null pointers\n");
	}
}

void rem_edf_task_list(struct edf_rq *rq, struct task_struct *p)
{
	struct list_head *ptr=NULL,*next=NULL;
	struct edf_task *edf_task=NULL;

	if(rq && p){
		list_for_each_safe(ptr,next,&rq->edf_list_head){
			edf_task=list_entry(ptr,struct edf_task, edf_list_node);
			if(edf_task){
				if(edf_task->task->edf_id == p->edf_id){
					list_del(ptr);
					kfree(edf_task);
					return;
				}
			}
		}
	}
}


static void enqueue_task_edf(struct rq *rq, struct task_struct *p,int wakeup)
{
  struct edf_task *t=NULL;
  if(p){
    t=find_edf_task_list(&rq->edf_rq,p);
    if(t){
      //  t->absolute_deadline=sched_clock()+p->deadline;
 t->absolute_deadline=p->absolute_deadline;
      insert_edf_task_rb_tree(&rq->edf_rq,t);
      atomic_inc(&rq->edf_rq.nr_running);
    }
    else{
      printk(KERN_ALERT "enqueue_task_edf\n");
    }
  }
}

static void dequeue_task_edf(struct rq *rq, struct task_struct *p, int sleep)
{
  struct edf_task *t=NULL;
  if(p){
    t=find_edf_task_list(&rq->edf_rq,p);
    if(t){
      remove_edf_task_rb_tree(&rq->edf_rq,t);
      atomic_dec(&rq->edf_rq.nr_running);
      if(t->task->state==TASK_DEAD||t->task->state==EXIT_DEAD||t->task->state==EXIT_ZOMBIE){
	rem_edf_task_list(&rq->edf_rq,t->task);
      }
    }
    else{
      printk(KERN_ALERT "dequeue_task_edf\n");
    }
  }
}


static void check_preempt_curr_edf(struct rq *rq, struct task_struct *p)
{
  struct edf_task *t=NULL, *curr=NULL;
  /* if(atomic_read(&rq->edf_rq.nr_running)&&rq->curr->policy!=SCHED_EDF){
    resched_task(rq->curr);
  }
  else{*/
    t=earliest_deadline_edf_task_rb_tree(&rq->edf_rq);
    if(t){
      curr=find_edf_task_list(&rq->edf_rq,rq->curr);
      if(curr){
	if(t->absolute_deadline < curr->absolute_deadline)
	  resched_task(rq->curr);
      }
      else{
	printk(KERN_ALERT "check_preempt_curr_edf\n");
      }
      //  }
  }

}


static struct task_struct *pick_next_task_edf(struct rq *rq)
{
  struct edf_task *t=NULL;
  t=earliest_deadline_edf_task_rb_tree(&rq->edf_rq);
  if(t){
    //get random number and 80% percent
    int i,j;
    get_random_bytes(&i,sizeof(int));
    get_random_bytes(&j,sizeof(int));
    if (i>j)
    return t->task;
    else
      return NULL;
  }
  return NULL;
}














static const struct sched_class edf_sched_class = {
  .next      =&fair_sched_class,
  .enqueue_task   =enqueue_task_edf,
  .dequeue_task   =dequeue_task_edf,

  .check_preempt_curr   =check_preempt_curr_edf,

  .pick_next_task   =pick_next_task_edf,

  //.set_curr_task   =set_curr_task_edf,
  // .task_tick   =task_tick_edf,
};
