#include <stdio.h>
#include <unistd.h>

unsigned int func(unsigned int counter) {
	counter++;
	return counter;
}

void main(){
	unsigned int counter=0;
	pid_t pid=getpid();
	printf("pid = %d\n",pid);
	while(1) {
		unsigned int ret=func(counter);
		printf("func=%d\n",ret);
		counter++;
		sleep(1);
	}
}
