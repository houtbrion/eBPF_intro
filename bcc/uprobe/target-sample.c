#include <stdio.h>
#include <unistd.h>

unsigned int func(unsigned int counter) {
	return counter;
}

void main(){
	unsigned int counter=0;
	pid_t pid=getpid();
	printf("pid = %d\n",pid);
	while(1) {
		counter++;
		unsigned int ret=func(counter);
		printf("ret=%d\n",ret);
		sleep(1);
	}
}
