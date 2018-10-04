#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm-generic/uaccess.h>
#include <linux/list.h>
#include <linux/spinlock.h>



MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("modlist Kernel Module - FDI-UCM");
MODULE_AUTHOR("Francisco Denis y Diego Laguna");

DEFINE_SPINLOCK(sp); //Definimos el spinlock 

#define MAX_CHARS 256
#define BUFFER_LENGTH       PAGE_SIZE


static struct proc_dir_entry *proc_entry;

struct list_head mylist; /* Lista enlazada */

/* Nodos de la lista */
typedef struct {
	int data;
	struct list_head links;
}list_item_t;

static char *modlist;  // Space for the "modlist"

void add_list(int num){
	list_item_t *item;
	item = vmalloc(sizeof(list_item_t));
	item->data=num;
	spin_lock(&sp);   // Bloqueamos
	list_add_tail(&item->links,&mylist);	
	spin_unlock(&sp);   // Desbloqueamos
} 

void remove_list(int num){
	struct list_head* cur_node=NULL;
	list_item_t* item=NULL;
	struct list_head* aux;
	spin_lock(&sp);  //Bloqueamos
	list_for_each_safe(cur_node,aux,&mylist){
		item = list_entry(cur_node,list_item_t, links);
		if (item->data == num){
		list_del(cur_node);
		vfree(item);
		}
	}
	spin_unlock(&sp);  //Desbloqueamos
}

void cleanup(void){
	struct list_head* cur_node=NULL;
	list_item_t* item=NULL;
	struct list_head* aux;
	spin_lock(&sp);  //Bloqueamos
	list_for_each_safe(cur_node,aux,&mylist){
		item = list_entry(cur_node,list_item_t, links);
		list_del(cur_node);
		vfree(item);
	}
	spin_unlock(&sp);  //Desbloqueamos
}
		

static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
 int num;
 char kbuf[MAX_CHARS];

  if ((*off) > 0) /* The application can write in this entry just once !! */
    return 0;
  
  if (len > MAX_CHARS) {
    printk(KERN_INFO "modlist: not enough space!!\n");
    return -ENOSPC;
  }
  
  /* Transfer data from user to kernel space */
  if (copy_from_user(kbuf,buf,len))
	return -EFAULT;

  kbuf[len]='\0';
	
  if (sscanf(kbuf, "add %i",&num)==1) add_list(num);

  else if (sscanf(kbuf, "remove %i",&num)==1) remove_list(num);
  
  else {
  	char palabra[100];
  	strcpy(palabra,kbuf);
  	if (strcmp(palabra, "cleanup\n")==0) cleanup();
  	else printk("Unknown command");	
  }
   
  return len;
}

static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {

  char kbuf[MAX_CHARS];
  int nr_bytes = 0;
  struct list_head* cur_node=NULL;
  list_item_t* item=NULL;

  if ((*off) > 0)  //Tell the application that there is nothing left to read 
      return 0;  

  spin_lock(&sp);
  list_for_each(cur_node,&mylist){
	item = list_entry(cur_node,list_item_t, links);
	sprintf(&kbuf[nr_bytes],"%d\n",item->data);
	nr_bytes = strlen(kbuf);
	trace_printk("%s",kbuf);
  }
  spin_unlock(&sp);

    
  if (len<nr_bytes)
    return -ENOSPC;
  
    /* Transfer data from the kernel to userspace */  


  if (copy_to_user(buf, kbuf,nr_bytes))
    return -EINVAL;
    

 (*off)+=len;  /* Update the file pointer */

  return nr_bytes; 
}

static const struct file_operations proc_entry_fops = {
    .read = modlist_read,
    .write = modlist_write,    
};



int init_modlist_module( void )
{
  int ret = 0;
  INIT_LIST_HEAD(&mylist);
  modlist = (char *)vmalloc( BUFFER_LENGTH );

  spin_lock_init(&sp);  //Iniciamos el spinlock
  

if (!modlist) {
    ret = -ENOMEM;
  } else {

    memset( modlist, 0, BUFFER_LENGTH );
    proc_entry = proc_create( "modlist", 0666, NULL, &proc_entry_fops);
    if (proc_entry == NULL) {
      ret = -ENOMEM;
      vfree(modlist);
      printk(KERN_INFO "modlist: Can't create /proc entry\n");
    } else {
      printk(KERN_INFO "modlist: Module loaded\n");
    }
  }

  return ret;

}


void exit_modlist_module( void )
{	
  if(!list_empty(&mylist)){
	cleanup();
	printk(KERN_INFO "Calling cleanup() before exit...\n");
  }	
  remove_proc_entry("modlist", NULL);
  vfree(modlist);
  printk(KERN_INFO "modlist: Module unloaded.\n");
}


module_init( init_modlist_module );
module_exit( exit_modlist_module );
