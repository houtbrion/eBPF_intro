#include <stdio.h>
#include <sys/sdt.h>
#include <unistd.h>
void main(){
	unsigned int counter=0;
	pid_t pid=getpid();
	printf("pid = %d\n",pid);
	while(1) {
		counter++;
		printf("counter=%d\n",counter);
		// probeの引数として2つあげるので，DTRACE_PROBE2を使う
		DTRACE_PROBE2(foo, bar, counter, "hello");
		sleep(1);
	}
}
