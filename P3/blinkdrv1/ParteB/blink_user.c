#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/errno.h>


int main(){
	int a = 1;
	int file = open("/dev/usb/blinkstick0",O_WRONLY | O_APPEND);

	const void *luz1 = "0:0x1D334A,1:0x1D334A,2:0x1D334A,3:0x1D334A,4:0xCC0605,5:0xCC0605,6:0xCC0605,7:0xCC0605";
	const void *luz1b = "0:0x1D334A,1:0x1D334A,2:0x1D334A,3:0x1D334A";
	const void *luz2 = "0:0xCC0605,1:0xCC0605,2:0xCC0605,3:0xCC0605,4:0x1D334A,5:0x1D334A,6:0x1D334A,7:0x1D334A";
	const void *luz2b = "4:0xCC0605,5:0xCC0605,6:0xCC0605,7:0xCC0605";
	const void *luz3 = "0:0xD6AE01";
	const void *luz4 = "0:0xD6AE01,1:0xD6AE01";
	const void *luz5 = "0:0xD6AE01,1:0xD6AE01,2:0xD6AE01";
	const void *luz6 = "0:0xD6AE01,1:0xD6AE01,2:0xD6AE01,3:0xD6AE01";
	const void *luz7 = "0:0xD6AE01,1:0xD6AE01,2:0xD6AE01,3:0xD6AE01,4:0xD6AE01";	
	const void *luz8 = "0:0xD6AE01,1:0xD6AE01,2:0xD6AE01,3:0xD6AE01,4:0xD6AE01,5:0xD6AE01";
	const void *luz9 = "0:0xD6AE01,1:0xD6AE01,2:0xD6AE01,3:0xD6AE01,4:0xD6AE01,5:0xD6AE01,6:0xD6AE01";
	const void *luz10 = "0:0xD6AE01,1:0xD6AE01,2:0xD6AE01,3:0xD6AE01,4:0xD6AE01,5:0xD6AE01,6:0xD6AE01,7:0xD6AE01";

	if(file < 0){
		printf("No se pudo leer");
		return -1;
	}
	while(a!=0){
		int b = 0;
		while(b<=1){
			write(file,luz1,87);
			usleep(195000);
			write(file,luz2,87);
			usleep(195000);
			write(file,luz1,87);
			usleep(195000);
			write(file,luz2,87);
			usleep(195000);
			write(file,luz1,87);
			usleep(195000);
			write(file,luz2,87);
			usleep(195000);
			b++;
		}
		b=0;
		while(b<=2){
			write(file,luz1b,43);
			usleep(195000);
			write(file,"",0);
			usleep(125000);
			write(file,luz1b,43);
			usleep(125000);
			write(file,luz2b,43);
			usleep(125000);
			write(file,"",0);
			usleep(125000);
			write(file,luz2b,43);
			usleep(125000);
			b++;
		}
		
		write(file,luz3,10);
		usleep(100000);
		write(file,luz4,21);
		usleep(100000);
		write(file,luz5,32);
		usleep(100000);
		write(file,luz6,43);
		usleep(100000);
		write(file,luz7,54);
		usleep(100000);
		write(file,luz8,65);
		usleep(100000);
		write(file,luz9,76);
		usleep(100000);
		write(file,luz10,87);
		usleep(100000);
	 }

	return 0;

}
