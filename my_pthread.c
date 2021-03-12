// File:	rpthread.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "rpthread.h"

//PSEUDOCODE:
/*
1.start program
2. main run
3. main call rpthread_create
4. in phtread_create, init scheduler+ timer+ sighandlers+ etc
5. create contect for new thread+ current funning funct (main)
6. getcontect for both, makecontext for new thread (allocate stack)
7. create scheduler (optional), place in some queues and return with pthread_create
8. main still running
9. timer interrupt
10. save context into main context
11.swap scheduler (with swapcontext)
*/

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE

tQueue *readyQueue;

tQueue *exitQueue;

tQueue *joinQueue;

tQueue *waitQueue;//this one is for mutex


tNode *currentNode = NULL;

int firstThread = 1;
int status = -1; //0:ready, 1:scheduled,2:blocked

//var to use in schedule
int freeCalled = 0; // if 1, run free
int mutexCalled = 0; //if 1, run mutex func
int exitCalled = 0; //if 1, run exit func
int joinCalled = 0; //if 1, run join func

//int parentAdded = 0; //if 1, there is parent

struct itimerval tv; //timer

/* create a new thread */
int rpthread_create(rpthread_t * thread, pthread_attr_t * attr, 
                      void *(*function)(void*), void * arg) {
       // create Thread Control Block
       // create and initialize the context of this thread
       // allocate space of stack for this thread to run
       // after everything is all set, push this thread int
       // YOUR CODE HERE
	
    	//if queue is empty, ie. this is the first thread, then we need to add the caller to the queue. This means that there needs to be a context made for the caller 

	/*
	tNode* node = malloc(sizeof(tNode*));
	//i added a '*,' error without *
	node->curtcb.tid = *thread; 
	
	node->curtcb.ctx = malloc(sizeof(ucontext_t*));
	getcontext(node->curtcb.ctx);
	node->curtcb.ctx->uc_stack.ss_size = SIGSTKSZ;
	node->curtcb.ctx->uc_stack.ss_sp = malloc(SIGSTKSZ); 
	//memset(node->curtcb.ctx->uc_stack.ss_sp, '/0', SIGSTKSZ);
	makecontext(node->curtcb.ctx, *function, arg);
	*/


	//WE WILL NEED TO MAKE THE SCHEDULER, and INIT_SCHEDULER

	//if queue empty
	if(firstThread == 1){

		status = READY; //make status into ready

		//tNode* parent= malloc(sizeof(tNode*));
		//set up thread node parent
		tNode* parent= malloc(sizeof(tNode*));
		parent->curtcb.ctx = malloc(sizeof(ucontext_t*));
		parent->nextNode = NULL;
		parent->jTids = NULL;
		parent->curtcb.tid = 0; 
		//define curtcb for parent
		//i added a '*,' error without *

		getcontext(parent->curtcb.ctx);

		init_schedule();

		currentNode = parent;//we will need this for identifying which node to add to exit, join, and mutex
		//i plan on using the currentNode to record the topNode in the Q, after parent is used

		//add parent tnode to queque
		enqueue(parent, readyQueue);
		
		//create child node
		tNode* child= malloc(sizeof(tNode*));
		child->curtcb.ctx = malloc(sizeof(ucontext_t*));

		child->nextNode = NULL;
		child->jTids = NULL;

		child->curtcb.tid = thread; 
		child->curtcb.ctx = malloc(sizeof(ucontext_t*));

		getcontext(child->curtcb.ctx);
		child->curtcb.ctx->uc_stack.ss_size = SIGSTKSZ;
		child->curtcb.ctx->uc_stack.ss_sp = malloc(SIGSTKSZ); 
		child->curtcb.ctx->uc_link = 0; 
		makecontext(child->curtcb.ctx, (void*)function,1, arg);
	
		parent->curtcb.ctx->uc_link = child->curtcb.ctx; 
		enqueue(child, readyQueue);
		schedule();

		//we need to make this function
		//though the schdule is for partII, but for now we can treat it as the runner

		return 0;
	}
	else{//queue exists

		//create child node
		tNode* child= malloc(sizeof(tNode*));
		child->nextNode = NULL;
		child->jTids = NULL;
		child->curtcb.tid = thread; 
		child->curtcb.ctx = malloc(sizeof(ucontext_t*));

		getcontext(child->curtcb.ctx);
		child->curtcb.ctx->uc_stack.ss_size = SIGSTKSZ;
		child->curtcb.ctx->uc_stack.ss_sp = malloc(SIGSTKSZ); 
		child->curtcb.ctx->uc_link = 0; 
		makecontext(child->curtcb.ctx, (void*)function,1, arg);

		//connect child node to target queue according to global var status
		if(status==READY){
			tQueue *targetQueue = readyQueue;
			enqueue(child, targetQueue);

		}

		return 0;
	}
}

/* give CPU possession to other user-level threads voluntarily */

//i am unsure how to implement this function...
/*
	My idea is that since we are swapcontext the scheduler so the scheduler thread
	can be enqueue the current threed back to runqueue
*/
int rpthread_yield() {
	
	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// wwitch from thread context to scheduler context
	// YOUR CODE HERE

	//using swapcontext()
	//status = SCHEDULED; //change from ready->schedule

	resetTime();

	schedule();

	return 0;
};

/* terminate a thread */
void rpthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE



	/*
		for this i am adding the parent node(currentNode) to exitQueue
		if the node is not found in the joinQueue, i will dequeue the node
		from readyQueue and add the node to exitqueue
	*/


	resetTime();

	//searchQ is looking for the threads that called join on it
	//thus search through joinQ
	//the function would remove the node from joinQ and enqueue it to readyQ
	if(searchQ(currentNode->curtcb.tid, joinQueue)==0){

		dequeue(readyQueue);//remove head from readyqueue
		freeCalled = 1;//thread exited from readyQ
	}

	else if(searchQ(currentNode->curtcb.tid, joinQueue) == -1){ //not found

		dequeue(readyQueue);//dequeue from 
		enqueue(currentNode, exitQueue); //add to exit queue
	}

	//WE NEED TO MAKE SCHEDULER TO RUN

	//make parent node's value_ptr equal to value_ptr
	//this works cause the currentNode is either added to exitQueue if not found
	//or value_ptr is not found, we add the value to currentNdoe's value_ptr
	currentNode->curtcb.value_ptr = value_ptr;

	exitCalled = 1; //to show exit funct called, so do not equeue readyQ
	schedule();
}


/* Wait for thread termination */
int rpthread_join(rpthread_t thread, void **value_ptr) {
	
	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE

	resetTime();

	/*
	for this, i am search for the tid in the exitQueue,
	if it is there (value_ptr is not null), the exiting thread's value will be
	PASSED BACK (i enqueued it to the joinqueue).
	*/

	//we will search exitQueue to see if target thread (with tid) has been exited

	if(searchQ(thread, exitQueue)== -1){
	//thread id is not found in exitqueue, meaning we remove node from readyQ, and add to joinQ
		currentNode = dequeue(readyQueue);//remove from readyqueue
		enqueue(currentNode, joinQueue);//added to joinQueue

		//makes new node and sets its tid to the one that we are waiting for 
		tidNode* newNode = malloc(sizeof(tidNode*));
		newNode->tid = thread;

		//if list of joined tids is empty
		if(currentNode->jTids == NULL){
			currentNode->jTids = newNode; 

		//insert onto the front
		} else {
			newNode->next = currentNode->jTids; 
			currentNode->jTids = newNode->next; 
		}
		//WE NEED SCHEDULE
		joinCalled = 1; 
		schedule();

		return 0;
	}
	//WE NEED SCHEDULE
}

/* initialize the mutex lock */
int rpthread_mutex_init(rpthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	//initialize data structures for this mutex

	// YOUR CODE HERE

	mutex->init = 1;
	mutex->lock = 0;//unlock lock for tNode
	mutex->lockQueue = 0;//unlock lock for tQueue

	mutex->wait = malloc(sizeof(tQueue*));
	mutex->wait->head = NULL;
	mutex->wait->tail = NULL;

	return 0 ;
}

/* aquire the mutex lock */
int rpthread_mutex_lock(rpthread_mutex_t *mutex) {
        // use the built-in test-and-set atomic function to test the mutex
        // if the mutex is acquired successfully, enter the critical section
        // if acquiring mutex fails, push current thread into block list and //  
        // context ` to the scheduler thread

        // YOUR CODE HERE

		if(mutex->init!=1){//mutex not yet initialized
			printf("ERROR: MUTEX not initialized");
			return -999;
		}
		
		while( __sync_lock_test_and_set(&mutex->lock, 1) == 1){//LOCKED

			mutex->lockQueue = 1; //lock the Qlock

			if(mutex->lock == 1) {//if mutex is locked

				dequeue(readyQueue);//remove head from readyQ
				enqueue(currentNode, mutex->wait);//enqueue the currentNode to mutex's wait

				mutex->lockQueue= 0; //unlock mutex's Qlock

				mutexCalled = 1;

				schedule();//run scheduler

			}
			else{//is mutex is unlocked, we unlock mutex's Qlock
				mutex->lockQueue = 0; //opens the mutex waitQ
			}
		}

        return 0;
};

/* release the mutex lock */
int rpthread_mutex_unlock(rpthread_mutex_t *mutex) {
	// Release mutex and make it available again. 
	// Put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE


	mutex->lockQueue = 1;//lock queue first, to stop new q's entering
	//create node from waitQ's head

	tNode *waitHead = dequeue(mutex->wait);

	mutex->lockQueue = 0; //unlock the waitQ, to make changes

	if(waitHead != NULL){//there exists thread in waitQ

		enqueue(waitHead, readyQueue);
		//append waitHead to readyQueue
	}

	return 0;
};


/* destroy the mutex */
int rpthread_mutex_destroy(rpthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in rpthread_mutex_init

	//we want to dump everything in mutex waitQ onto readyQ
	tNode *waitHead = mutex->wait->head;

	while(waitHead != NULL){

		enqueue(waitHead, readyQueue);

		waitHead = waitHead->nextNode;
	}

	free(mutex->wait);//free the entire waitQ

	//to destroy, undo init_mutex
	mutex->init = 0;
	mutex->lock = 0;
	mutex->lockQueue=0;
	mutex->wait = NULL;

	return 0;
}

// search funct, join needs enqueue, exit needs to free nodes
int searchQ(rpthread_t tid, tQueue *queue){

	tNode *curr = queue->head;
	tNode *prev = curr;

	int flag = 0; //1=join, 2=exit

	if(queue == joinQueue) flag = 1;
	if(queue == exitQueue) flag = 2;

	if(flag==0){//not joinQ or exitQ
		printf("ERROR\n");
		return -999;
	}

	if(curr == NULL) return -1; //return empty

	while(curr!= NULL){

		if(curr->curtcb.tid == tid){//node exists in joinQueue
			//enqueue the found node to readyqueue
			//then remove it from joinQueue
			if(curr == queue->head && curr->nextNode==NULL){ //still at the head and no more next
				
				curr->nextNode = NULL;

				if(flag==1){
					enqueue(curr, readyQueue);
				}

				queue->head = NULL;
				queue->tail = NULL;
			
				if(flag==2) free(curr);
			}
			else if(curr == joinQueue->head && curr->nextNode!=NULL){//at head, but next exists

				curr->nextNode = NULL;

				if(flag==1)	enqueue(curr, readyQueue);

				joinQueue->head = joinQueue->head->nextNode;

				if(flag==2) free(curr);
			}
			else if(curr->nextNode == NULL){//our target is the last node in joinqueue

				if(flag==1)enqueue(curr, readyQueue);

				joinQueue->tail = prev;

				if(flag==2) free(curr);

			}

			else{//in between two nodes

				prev->nextNode = curr->nextNode;//skip curr
				
				curr->nextNode = NULL;

				if(flag==1)enqueue(curr, readyQueue);
				if(flag==2) free(curr);
			}
			
			return 0; //node with the tid is found
			
		}
		prev = curr;
		curr = curr->nextNode;
	}
	//tid not found in joinQueue
	return -1;
}

//return 1 when tid is found in exit queue, 0 otherwise 
int searchExitQueue(rpthread_t tid){
	tNode* cur = exitQueue->head; 
	
	while(cur != NULL){
		if(cur->curtcb.tid == tid){
			return 1; 
		}
		cur = cur->nextNode; 
	}
	return 0; 
}

void remove_joinlist(tidNode** head, tidNode* toRemove){
	if((*head)->tid == toRemove->tid){
		free(head);
		*head = (*head)->next;  
		return; 
	}

	tidNode *cur = (*head)->next, *prev = (*head); 
	while(cur != NULL){
		if(cur->tid == toRemove->tid){
			toRemove->next = cur->next; 
			free(cur); 
			break; 
		}
		prev = cur; 
		cur = cur->next; 		
	}
}

//compares list of threads the node is waiting on to exitQueue, if found in exitQueue this means that thread is done so remove from node's joinList
void compare_joinlist(tNode* node){
	tidNode* head = node->jTids; 
	if(head == NULL){
		return; 
	}

	//not waiting on any threads
	if(head == NULL){
		return; 
	}

	//remove all tids found in exitQueue
	tidNode* cur = head; 
	while(cur != NULL){
		if(searchExitQueue(cur->tid)){
			//remove from jtids
			remove_joinlist(&head, cur);
			return; 
		}
		cur = cur->next; 
	}
}

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library 
	// should be contexted switched from thread context to this 
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (RR or MLFQ)

	// if (sched == RR)
	//		sched_rr();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

	//if join,exit,mutex, we do not equeue, cause function already added equeue
	//else, we equeue the tail of readyQ, cause the pthread was added to readyQ
	//for our algorithm adds nodes to the tail, but because the three Q were not changed,
	//the tail would be the 'first' node to be processed
	
	//setup for new thread input
	if(joinCalled==1) joinCalled = 0;
	
	else if(exitCalled==1) exitCalled = 0;

	else if(mutexCalled==1) mutexCalled=0;

	else{
		tNode *curr = dequeue(readyQueue);
		enqueue(curr, readyQueue);
	}

	//if threads are waiting for others to finish 
	if(joinQueue->head != NULL){
		compare_joinlist(joinQueue->head);
		if(joinQueue->head->jTids == NULL){
			tNode* temp = dequeue(joinQueue); 
			enqueue(temp, readyQueue);
		}
	}

	tNode *firstNode = readyQueue->head;

	if(firstNode == NULL){
		//if null means no thread processed
		printf("ERROR, no top thread node found in readyQ\n");
		return;
	}

	//if equal then parent.tid is the firstNode in readyQ
	if(firstNode->curtcb.tid == currentNode->curtcb.tid){

		if(firstNode->nextNode != NULL){
			firstNode = firstNode->nextNode;
			//move firstNode to nextNode in readyQ
			// this is later used to free
		}
	}

	tNode *temp = currentNode;//currentNode is parent
	currentNode = firstNode;//move current to first node

	if(freeCalled==1){//exiting thread freed
		//different form searchQ, we free node in exitQ
		//here we are freeing temp
		free(temp);
		freeCalled = 0;
		setcontext(firstNode->curtcb.ctx);
	}
	else{//swap temp's context with firstNode

		if(swapcontext(temp->curtcb.ctx, firstNode->curtcb.ctx) == -1){
			printf("AHHHH");
		}
	}

	// schedule policy
	//PART2
	#ifndef MLFQ
		// Choose RR
		// CODE 1
	#else 
		// Choose MLFQ
		// CODE 2
	#endif

}

//Append tNode to the end of tQueue
void enqueue(tNode* newNode, tQueue *queue){

	newNode->nextNode=NULL;
	if(queue->head == NULL){//empty queue
		queue->head = newNode;
		queue->tail = newNode;
	}
	else{
		queue->tail->nextNode = newNode;
		queue->tail->curtcb.ctx->uc_link = queue->tail->nextNode->curtcb.ctx;
		queue->tail = queue->tail->nextNode;
		
	}

}

//we remove the first tNode from targetqueue
//for mutex's waitQ and the readyQ
tNode* dequeue(tQueue *queue){

	tNode *current = queue->head;
	tNode *deqNode = NULL; //for extra cred

	if(queue->head == NULL) deqNode = NULL;

	else if(current->nextNode == NULL){

		deqNode = queue->head;
		queue->head = NULL;
		queue->tail = NULL;

	}
	//this means there is more than one tNode in queue
	else{
		deqNode = queue->head;
		queue->head = queue->head->nextNode;

		deqNode->nextNode = NULL;
	}
	return deqNode;//we can just treat this as a void
}

//for init queue
void starter(tQueue *queue){

	queue = malloc(sizeof(tQueue));
	queue->head = NULL;
	queue->tail = NULL;
}

//our initial runenqueue
//initializes out queues
void init_schedule(){

	readyQueue = malloc(sizeof(tQueue)); 
	readyQueue->head = NULL; 
	readyQueue->tail = NULL; 
	
	exitQueue = malloc(sizeof(tQueue)); 
	exitQueue->head = NULL; 
	exitQueue->tail = NULL; 
	
	joinQueue = malloc(sizeof(tQueue));
	exitQueue->head = NULL; 
	exitQueue->tail = NULL; 

	//signal for alarm
	signal(SIGALRM, sighandler);
}

void sighandler(int signum){//from 
	printf("Caught signal %d, schedule starting...]\n", signum);
	schedule();
}

void resetTime(){

	//setup time
	tv.it_value.tv_sec= 0;
	tv.it_value.tv_usec = 0;

	tv.it_interval = tv.it_value;
	//setup complete!
}


/* Round Robin (RR) scheduling algorithm */
static void sched_rr() {
	// Your own implementation of RR
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE

}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}
