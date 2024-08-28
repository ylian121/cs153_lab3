#include "types.h"
#include "stat.h"
#include "user.h"


struct shm_cnt {
	struct unspinlock lock;
	int cnt;
}; 

int main(int argc, char *argv[]) {
	int i = 0; 
	struct shm_cnt *counter;

	int pid = fork(); //return 0 in the child process, and the PID of the child in the parent 
	
	if (pid < 0) {
		printf(1, "fork failed\n"); 
		exit();
	} 

	//both processes open the shared memory segment
	if (shm_open(1, (char **)&counter) < 0) {
		printf(1, "shm_open failed in %s\n", pid ? "parent" : "child"); 
		exit(); 
	}

	printf(1, "%s process successfully opened shared memory. Counter address: %x\n", pid ? "parent" : "child", counter);

	for (i = 0; i < 10000; i++) {
		uacquire(&(counter->lock));
		counter->cnt++;
		urelease(&(counter->lock));

		if (i % 1000 == 0) {
			printf(1, "counter in %s is %d at address %x\n", pid ? "parent" : "child", counter->cnt, counter);
		}
	}

	if (pid) {
		wait();
		printf(1, "final counter in parent is %d\n", counter->cnt);
	} else { 
		printf(1, "final counter in child is %d\n\n", counter->cnt);
	}

	//please insert this line of code below once shm_close is implemented
	//shm_close(1);
		
	exit();
	return 0;
}	




