#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/kfifo.h>
#include <linux/vmalloc.h>
#include <linux/random.h>
#include <linux/spinlock.h>
#include <asm-generic/uaccess.h>



MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("modtimer module");
MODULE_AUTHOR("Francisco Denis Herrera y Diego Laguna Martin");

#define MAX_SIZE 32
#define BUF_SIZE 256

DEFINE_SPINLOCK(sp);	//Spin de buffereo

static struct proc_dir_entry *proc_modtimer;
static struct proc_dir_entry *proc_modconfig;
struct work_struct transfer_task;
struct timer_list my_timer; /* Structure that describes the kernel timer */
struct kfifo cbuffer; /* Buffer circular */
struct list_head mylist;
struct work_struct work;

unsigned long flags;
unsigned long timer_period_ms = 500;
int emergency_threshold = 75; 
unsigned int max_random = 300;
int prod_count = 0;  /* Número de procesos que abrieron la entrada /proc para escritura (productores) */
int cons_count = 0; /* Número de procesos que abrieron la entrada proc para lectura (consumidores) */
struct semaphore mtx;	/* para garantizar Exclusión Mutua */
struct semaphore sem_cons; /* cola de espera para consumidor(es) */
int nr_cons_waiting=0; /* Número de procesos consumidores esperando */

struct list_item_t {
	unsigned int data;
	struct list_head links;
} list_item_t;

void copy_items_into_list(struct work_struct *work ){

    	struct list_item_t *node;
    	int len;
	unsigned int kbuffer[MAX_SIZE];
	int i = 0;
	int resultado;
	int aux;
    
    	spin_lock_irqsave(&sp,flags);
	len = kfifo_len(&cbuffer);
	resultado = kfifo_out(&cbuffer,kbuffer,len);
	spin_unlock_irqrestore(&sp,flags);
	aux = len/4;
	down(&mtx);


	spin_lock_irqsave(&sp,flags);
		


	spin_unlock_irqrestore(&sp,flags);

	while(aux > 0){	

		node = (struct list_item_t *)vmalloc(sizeof(struct list_item_t));
		node->data = kbuffer[i];
		spin_lock_irqsave(&sp,flags);
		list_add_tail(&node->links, &mylist);
		spin_unlock_irqrestore(&sp,flags);
		i++;
		aux--;
       	}

	 printk(KERN_INFO "%d items moved from buffer to the list", i);
	    

	if(nr_cons_waiting > 0){
       		up(&sem_cons);
        	nr_cons_waiting--;
    	}
    
    	up(&mtx);
}



static void fire_timer(unsigned long data){
	
	int len,percent;
	unsigned int random;
	random = (get_random_int()%max_random);
	printk(KERN_INFO "Numeraco generado %u\n",random);

	spin_lock_irqsave(&sp,flags);

	kfifo_in(&cbuffer,&random,sizeof(unsigned int));
	len = kfifo_len(&cbuffer);
	
	spin_unlock_irqrestore(&sp,flags);

	percent = (100*len)/MAX_SIZE;
	
	if(percent >= emergency_threshold){
		int cpu_actual;
		int cpu_trabajo;
		cpu_actual = smp_processor_id();
		if(cpu_actual == 0) cpu_trabajo = 1;
		else cpu_trabajo = 0;

		INIT_WORK(&work,copy_items_into_list);
        
	        /* Cola de trabajo */
	        schedule_work_on(cpu_trabajo,&work);	
	}

	mod_timer( &(my_timer), jiffies + msecs_to_jiffies(timer_period_ms));
}

static ssize_t modtimer_read(struct file *filp, char __user *buf, size_t len, loff_t *off){

	struct list_item_t* node=NULL;
  	struct list_head *cur_node=NULL;
	struct list_head *aux;
    	char kbuf[BUF_SIZE];    
    	int nr_bytes=0;

    	down(&mtx); 

	while(list_empty(&mylist)){
        	nr_cons_waiting++;
        	up(&mtx);
		if (down_interruptible(&sem_cons)){
            		nr_cons_waiting --;
            		return -EINTR;  
        	}
        	down(&mtx);  
       	}

	list_for_each_safe(cur_node, aux, &mylist){
        	node=list_entry(cur_node, struct list_item_t, links);
		sprintf(&kbuf[nr_bytes],"%u\n",node->data);
		nr_bytes = strlen(kbuf);
		list_del(cur_node);
        	vfree(node);
    	}

    	up(&mtx);

	if (len<nr_bytes) return -ENOSPC;
	
	if (copy_to_user(buf, kbuf,nr_bytes)) return -EINVAL;

	(*off)+=len;

	return nr_bytes;
}
    

static int modtimer_open(struct inode *inode, struct file *file){
    
    /* Initialize field */
    my_timer.data=0;
    my_timer.function=fire_timer;
    my_timer.expires=jiffies + timer_period_ms + HZ;
    /* Activate the timer for the first time */
    add_timer(&my_timer);
    
    return 0;
}

static int modtimer_release(struct inode *inode, struct file *file){

	struct list_item_t* node;
	struct list_head *cur_node=NULL;
	struct list_head *aux=NULL;

	del_timer_sync(&my_timer);
 	flush_scheduled_work();

	spin_lock_irqsave(&sp,flags);

    	kfifo_reset(&cbuffer);

    	spin_unlock_irqrestore(&sp,flags);

	list_for_each_safe(cur_node, aux, &mylist){
        	node=list_entry(cur_node, struct list_item_t, links);
         	list_del(cur_node);
         	vfree(node);
    	}

	return 0;

}

static ssize_t modconfig_read(struct file *filp, char __user *buf, size_t len, loff_t *off){
    
    static bool end = false;
    char kbuf[BUF_SIZE];
    int nr_bytes;
    
    if (end){
            end = false;
            return 0;
    }

    nr_bytes=sprintf(kbuf,"timer_period_ms=%lu\nemergency_threshold=%i\nmax_random=%i\n",timer_period_ms, emergency_threshold, max_random);
    
    end=true;
    
    if (copy_to_user(buf, kbuf,nr_bytes))
        return -EFAULT;
        
    return nr_bytes;    
}

static ssize_t modconfig_write(struct file *filp, const char __user *buf, size_t len, loff_t *off){
    unsigned long num;
    int num2;
    char kbuf[BUF_SIZE];
    
    if (copy_from_user(kbuf, buf, len)) return -EFAULT;

    if (sscanf(kbuf, "timer_period_ms %lu", &num) == 1){
        timer_period_ms=num;
        printk(KERN_INFO "New timer period value saved successfuly %lu\n",timer_period_ms); 
    }
    
    if (sscanf(kbuf, "emergency_threshold %i", &num2) == 1){
        if (num2 <= 100 && num2 > 0){
            emergency_threshold = num2;
            printk(KERN_INFO "New timer period value saved successfuly %i\n",emergency_threshold); 
        }else{
            printk(KERN_ALERT "Value beteween 1-100 \n");
        }
    }
    
    if (sscanf(kbuf, "max_random %i", &num2) == 1){
        max_random = num2;
        printk(KERN_INFO "New max random value saved successfully %i\n",max_random);  
    }

    return len;
}


static const struct file_operations proc_entry_modtimer = {
    .read = modtimer_read, 
    .open    = modtimer_open,
    .release = modtimer_release,    
};

static const struct file_operations proc_entry_modconfig = {
    .read = modconfig_read,
    .write = modconfig_write,
};




int modtimer_init(void){
	
	//INICIALIZAR KFIFO
	int ret=0;
	ret = kfifo_alloc(&cbuffer,MAX_SIZE,GFP_KERNEL);
	if(ret){
		printk(KERN_ERR "kfifo_alloc error\n");
		return -ENOMEM;
	}

	//INICIALIZAR TIMER
 	init_timer(&my_timer);


	//INICIALIZAR LISTA
	INIT_LIST_HEAD(&mylist);

//INIT SEMAFOROS
sema_init(&sem_cons,0);
sema_init(&mtx,1);	
	//ENTRADAS PROC
	proc_modconfig = proc_create_data("modconfig", 0666, NULL, &proc_entry_modconfig,NULL);
    	proc_modtimer = proc_create_data("modtimer", 0666, NULL, &proc_entry_modtimer,NULL);
	if (proc_modconfig == NULL || proc_modtimer == NULL) {
		kfifo_free(&cbuffer);
		printk(KERN_INFO "Imposibilidad en la creacion de las entradas en proc\n");
		return  -ENOMEM;
	}


	printk(KERN_INFO "Modfifo: Cargado el Modulo.\n");
	return ret;
}



void modtimer_clean(void){

	kfifo_free(&cbuffer);	
	del_timer_sync(&my_timer);
	remove_proc_entry("modtimer", NULL);
	remove_proc_entry("modconfig", NULL);
	 
	printk(KERN_INFO "MODTIMER: Module released!\n");
}

module_init(modtimer_init);
module_exit(modtimer_clean);
