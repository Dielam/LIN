#include <linux/errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>

#ifdef __i386__
#define __NR_LEDCTL 353
#else
#define __NR_LEDCTL 316
#endif


long ledctl(unsigned int leds) {

	return (long) syscall(__NR_LEDCTL,leds);

}


int main(int argc, char *argv[]){

	unsigned int leds;
	int c = sscanf(argv[1],"%x",&leds);
	if (c != 1){
		printf("Formato erroneo\n");
		return 1;
	}
	if(leds<0 || leds>7){
		printf("El numero debe estar entre 0 y 7\n");
		return 2;
	}
	long res;
	res = ledctl(leds); //Si es 0 bien, si no es 0 accedes a perror
	if (res != 0){
		perror("ERROR: ");
		return 3;
	}
	return 0;

}
