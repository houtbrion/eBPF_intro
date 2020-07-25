#include <stdio.h>
#include <unistd.h>

struct MyStruct {
	int y[4];
};

void test_struct(struct MyStruct *var){
	printf(" y[] = %d, %d, %d, %d\n",
			var->y[0],
			var->y[1],
			var->y[2],
			var->y[3]
			);
}

void main(){
	struct MyStruct myStruct;
	for (int i=0; i<4 ; i++) {
		myStruct.y[i]=i+1;
	}
	while (1) {
		for (int i=0; i<4 ; i++) {
			test_struct(&myStruct);
			sleep(1);
		}
	}
}
