#include <stdlib.h>
int main(void* args){
	int a = 0;
	int* b = &a;
	printf("%p,%p,%d",b,&b,*b);

}
