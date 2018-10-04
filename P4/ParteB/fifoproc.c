#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm-generic/uaccess.h>
#include <asm-generic/errno.h>
#include <linux/semaphore.h>
#include <linux/kfifo.h>

#define MAX_SIZE 64

static struct proc_dir_entry *proc_entry; //Para crear entrada proc

struct kfifo cbuffer; /* Buffer circular */

int prod_count = 0; /* Número de procesos que abrieron la entrada /proc para escritura (productores) */

int cons_count = 0; /* Número de procesos que abrieron la entrada proc para lectura (consumidores) */

struct semaphore mtx; /* para garantizar Exclusión Mutua */

struct semaphore sem_prod; /* cola de espera para productor(es) */

struct semaphore sem_cons; /* cola de espera para consumidor(es) */

int nr_prod_waiting=0; /* Número de procesos productores esperando */

int nr_cons_waiting=0; /* Número de procesos consumidores esperando */



/* Se invoca al hacer open() de entrada /proc */
static int fifoproc_open(struct inode *inode, struct file *file){

	if (down_interruptible(&mtx)) return -EINTR;
		
	
	if(file->f_mode & FMODE_READ){
		
		cons_count++;
		while(prod_count == 0){
			up(&mtx);
			if (down_interruptible(&sem_cons)) return -EINTR;
		}
		up(&sem_prod);
	}

	else{

		prod_count++;
		while(cons_count == 0){
			up(&mtx);
	 		if (down_interruptible(&sem_prod)) return -EINTR;
		}
		up(&sem_cons);
	}

	up(&mtx);
	return 0;

}
/* Se invoca al hacer close() de entrada /proc */
static int fifoproc_release(struct inode *inode, struct file *file){
	
	if (down_interruptible(&mtx)) return -EINTR;

	if(file->f_mode & FMODE_READ){
		
		cons_count--;
		if(nr_prod_waiting != 0) up(&sem_prod);
		up(&sem_cons);
	}
		
	else{
		prod_count--;
		if(nr_cons_waiting != 0) up(&sem_cons);
		up(&sem_prod);
	}
		
	if(nr_prod_waiting == 0 && nr_cons_waiting == 0) kfifo_reset(&cbuffer);

	up(&mtx);
	return 0;
}


/* Se invoca al hacer read() de entrada /proc */
static ssize_t fifoproc_read(struct file *file, char * buff, size_t len, loff_t *off){
	char kbuffer[MAX_SIZE];
	int bt;

	if(len > MAX_SIZE) return -ENOSPC;

	if (down_interruptible(&mtx)) return -EINTR;
	

	while(kfifo_len(&cbuffer) < len && prod_count > 0){	//Si kfifo_len es menor y hay un productor
		
		nr_cons_waiting++;
		
		up(&mtx);

		if (down_interruptible(&sem_cons)) {
			down(&mtx);
			nr_cons_waiting--;
			up(&mtx);
			return -EINTR;
		}

		if (down_interruptible(&mtx)) {
			return -EINTR;
		}
	}
	
	if(prod_count == 0 && kfifo_is_empty(&cbuffer)){
		up(&mtx);
		return 0;
	} 
		
	bt = kfifo_out(&cbuffer,kbuffer,len);

	if (bt!=len)
		return -EINVAL;

	if (copy_to_user(buff,kbuffer,len)){
		return -EINVAL;
	}

	up(&sem_prod);

	up(&mtx);

	return len;
	
}
/* Se invoca al hacer write() de entrada /proc */
static ssize_t fifoproc_write(struct file *file, const char * buff, size_t len, loff_t *off){
	char kbuffer[MAX_SIZE];
	
	if(len > MAX_SIZE) return -ENOSPC;

	if (copy_from_user(kbuffer,buff,len)) return -EFAULT;
	
	if (down_interruptible(&mtx)) return -EINTR;


	while(kfifo_avail(&cbuffer)<len && cons_count > 0){

		nr_prod_waiting++;

		up(&mtx);

		/* Bloqueo en cola de espera */
		if (down_interruptible(&sem_prod)) {
			down(&mtx);
			nr_prod_waiting--;
			up(&mtx);
			return -EINTR;
		}

		/* Readquisición del 'mutex' antes de entrar a la SC */
		if (down_interruptible(&mtx)) {
			return -EINTR;
		}
	}

	

	if(cons_count == 0){ // && kfifo_is_full(&cbuffer) != 0
		up(&mtx);
		return -EPIPE;
	}

	kfifo_in(&cbuffer,kbuffer,len);

	up(&sem_cons);

	up(&mtx);

	return len;
}


static const struct file_operations proc_entry_fops = {
	.read = fifoproc_read,
	.write = fifoproc_write,    
	.release = fifoproc_release,
	.open = fifoproc_open,		
};

/* Funciones de inicialización y descarga del módulo */
int init_fifoproc_module( void ){

	int ret;
	ret = kfifo_alloc(&cbuffer,MAX_SIZE,GFP_KERNEL);
	if(ret){
		printk(KERN_ERR "kfifo_alloc error\n");
		ret = -ENOMEM;
		return ret;
	}

	sema_init(&sem_prod,0);
	sema_init(&sem_cons,0);
	sema_init(&mtx,1);

	
	proc_entry = proc_create_data("modfifo",0666, NULL, &proc_entry_fops, NULL);
	if (proc_entry == NULL) {
		kfifo_free(&cbuffer);
		printk(KERN_INFO "Modfifo: Imposibilidad en la creacion de la entrada en proc\n");
		return  -ENOMEM;
	}

	printk(KERN_INFO "Modfifo: Cargado el Modulo.\n");
	return 0;

}


void cleanup_fifoproc_module( void ){

	remove_proc_entry("modfifo", NULL);
	kfifo_free(&cbuffer);
	printk(KERN_INFO "Modfifo: Modulo descargado.\n");

}

module_init( init_fifoproc_module );
module_exit( cleanup_fifoproc_module );
