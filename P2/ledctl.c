#include <linux/init.h>
#include <linux/tty.h>      //For fg_console 
#include <linux/kd.h>        //For KDSETLED 
#include <linux/vt_kern.h> 
#include <linux/syscalls.h>
#include <linux/kernel.h>

struct tty_driver* kbd_driver= NULL;


static inline int set_leds(struct tty_driver* handler, unsigned int mask){
    return (handler->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED,mask);
}

//Funcion para poder tener el puntero al driver del teclado
struct tty_driver* get_kbd_driver_handler(void){
   return vc_cons[fg_console].d->port.tty->driver;
}


SYSCALL_DEFINE1(ledctl,unsigned int,leds)
{
	kbd_driver = get_kbd_driver_handler();
	unsigned int bit0 = leds & 0x1;
	unsigned int bit1 = (leds & (1<<2)) >> 1;
	unsigned int bit2 = (leds & (1<<1)) << 1;
	leds = bit2 | bit1 | bit0;
	return set_leds(kbd_driver,leds);
}
