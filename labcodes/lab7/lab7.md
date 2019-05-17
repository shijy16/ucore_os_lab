# LAB 7 Report

2016011395 石景宜

## 练习 1  理解内核级信号量的实现和基于内核级信号量的哲学家就餐问题

### 内核级信号量的实现

内核级信号量主要通过一个等待队列数据结构和一个信号量实现，主要实现在`sem.c`中，现对其中的`up`、`down`和`try_down`操作。在操作过程中，必须关中断来保护这个过程。

#### `sem_init`

初始化信号量。

#### `up`

对应信号量的V操作。进入临界区，若当前等待队列不为空，则直接唤醒队列头的进程，不需要改变信号量的值；否则，信号量加一。然后退出临界区。

#### `down`

对应信号量的P操作。进入临界区，

+ 若当前信号量大于0，则减一并退出临界区，然后退出。
+ 若信号量不大于0，调用`wait_current_set`函数将当前进程加入等待队列并将状态设置为`PROC_SLEEPING`，调用`schedule`主动放弃CPU的占用。当进程被再次唤醒时，从等待队列中删去当前进程，坚持进程状态是否出错。

#### `try_down`

相当于尝试进行P操作，若当前信号量大于0，即不需要等待即可进行，将信号量减一后返回成功，执行相应代码。否则返回失败。

#### 执行流程

首先初始化信号量，进程需要访问共享变量时，首先执行up操作，进程继续运行或进入SLEEP状态等待。共享变量访问完毕后，进程执行down操作时。

### 用户态进程提供信号量机制设计方案

将信号量的申请、初始化、up、down、释放等操作全部封装为系统调用。由于用户态进程不能直接访问信号量，需要在申请信号量时给用户提供一个信号量索引而不是信号量本身。

## 练习2: 完成内核级条件变量和基于内核级条件变量的哲学家就餐问题

### 代码设计实现

参照注释可直接实现管程。

三个信号量的含义：

+ next 因调用signal而睡眠的进程使用next信号量控制。
+ mutex 控制管程的互斥访问。
+ sem 条件变量的互斥访问。

##### cond_signal

若当前有执行cond_wait而睡眠的进程(即睡眠在sem上)，可直接返回，否则唤醒该进程，自己进行睡眠，在自己睡眠时，需要让自己通过next信号量进行睡眠。

##### cond_wait

首先cv.count增加一，表示因等待该条件变量而睡眠的进程增加。然后判断是否有进程睡眠在next上，如果有则唤醒之，如果没有，则唤醒睡在mutex上的进程。然后让自己通过sem进行睡眠。

##### phi_take_forks_condvar

进入管程后，首先将当前哲学家状态置为HUNGRY，然后判断是否可以直接拿到左右两边的叉子，若不能，则睡眠，直到状态为EATING（旁边的哲学家在放下叉子时会判断是否可以让两边饥饿的哲学家进餐）。最后退出管程。

##### phi_put_forks_condvar

进入管程后，首先将当前哲学家状态改为THINKING,然后判断左右两边是否有哲学家可以进餐，然后退出管程。

### 与参考答案对比

我的实现和参考答案一致。

### 内核级条件变量的实现

在代码设计实现一节已有具体描述，不再赘述。

### 用户态进程/线程提供条件变量机制的设计方案

如果要在用户态实现条件变量，需要在用户态先实现信号量，在练习一中已有描述，不再赘述，信号量实现后，内核态的条件变量可直接在用户态中实现。

### 能否不用基于信号量机制来完成条件变量？

可以，由于信号量其实就是一个等待队列，故条件变量可以使用等待队列加上一个互斥锁实现。

##### cond_wait

1. 关中断，release锁
2. 将自己加入等待队列，设为SLEEP
3. 恢复中断
4. schedule
5. 关中断
6. 将自己移出等待队列
7. 恢复中断
8. acquire互斥锁

##### cond_signal

1. 关中断
2. 判断等待队列中是否有进程
   1. 有
      1. 移除第一个进程，唤醒之
   2. 没有
      1. do nothing
3. 恢复中断

这样的实现实际上就是把信号量的内部实现变成了条件变量的内部实现。

## 知识点总结

课程中有关知识点

+ 同步问题
+ 信号量
+ 条件变量
+ 管程
+ 哲学家问题

课程中其他重要知识点

+ 锁的实现
+ 死锁
+ 读者-写者问题