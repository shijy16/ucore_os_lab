# Lab6 Report

2016011395 石景宜

## 练习1: 使用 Round Robin 调度算法

#### 1. 请理解并分析sched_class中各个函数指针的用法，并结合Round Robin 调度算法描ucore的调度执行过程

##### 各个函数指针用途

+ init : 初始化调度器内部数据结构（链表、堆等），并初始化。
+ enqueue : 把进程放进调度队列，增加当前进程数。
+ dequeue : 将进程从队列中取出来，减少当前进程数。
+ pick_next ： 从队列中取出下一个被调度的进程。
+ proc_tick：通知调度器一个时间片结束，需要作出相应处理。

##### RR调度过程

+ 当进程做好运行准备的时候，会调用enqueue被放入调度队列尾部，放入时剩余时间片会被设置为初始值。
+ 当pick_next时，调度队列头的进程会dequeue，然后被调度。
+ 发生一次proc_tick时，当前进程剩余时间片会减少，当当前进程剩余时间片减少为0时，则先标记为need_resched，调度器将重新让当前进程enqueue，而后调用pick_next开始下一个进程。

#### 2. 请在实验报告中简要说明如何设计实现”多级反馈队列调度算法“，给出概要设计，鼓励给出详细设计

假设有n个优先级，调度器需要有n个队列，每一个队列对应一个优先级，同时，PCB需要记录优先级。优先级高的队列内进程拥有更多的时间片。初始时，进程拥有最高优先级。

+ init：初始化调度器
+ enqueue：进程进入队列时，按照进程剩余时间片数对进程优先级做下一步操作。
  + 剩余时间片为0，直接放入相应优先级队列。
  + 剩余时间片大于0，降低一级优先级后放入相应优先级队列。
+ dequeue：从相应优先级队列中取出该进程。
+ pick_next：按照调度算法选择一个优先级队列的队头进程。
+ proc_tick：当前进程时间片减一，若当前进程时间片为0，设置need_resched=1,通知调度器进行处理。

## 练习2: 实现 Stride Scheduling 调度算法

### 设计实现过程

直接按照注释实现如下：

+ init：初始化链表、lab_5_run_pool，proc_num
+ stride_enqueue：将进程插入可调度堆，进程个数加一，设置时间片为最大时间片个数，且把rq->lab6_run_pool置为堆头元素。
+ stride_dequeue：将进程从可调度堆中删除，进程数减一，且把rq->lab6_run_pool置为堆头元素。
+ stride_pick_next：若队列为空，则返回NULL，内核将调用idle。否则，进程lab_stride+= BIG_STRIDE/lab6_priority，返回堆头进程。
+ stride_proc_tick：若当前时间片大于0，则剩余时间片减一，否则置当前进程的need_resched为

### BIG_STRIDE的选取

在初始状态，明显有：$ SRIDE_{max} - STRIDE_{min} \le pass_{max} ​$

假设在第k个状态该不等式成立，则在第k+1个状态，选取$stride​$为最小值对应的进程。

则在第k+1个状态，有$ STRIDE_{min}'  \le  STRIDE_{min} + pass_{max}$,$ STRIDE_{min}' \ge STRIDE_{min}$

+ 若$ STRIDE_{min}' \le STRIDE_{max}​$ 则$ SRIDE_{max}' - STRIDE_{min}' = SRIDE_{max} - STRIDE_{min}' \le SRIDE_{max} - STRIDE_{min} \le pass_{max}​$
+ 若$ STRIDE_{min}' \gt STRIDE_{max}$,则$ SRIDE_{max}' - STRIDE_{min}' \le  (STRIDE_{min} + pass_{max}) - STRIDE_{min} = pass_{max} $

所以$ SRIDE_{max} - STRIDE_{min} \le pass_{max} $始终成立，则$ SRIDE_{max} - STRIDE_{min} \le BIG\_STRIDE $,所以只要任意STRIDE的差都可以表示真实大小关系，所以BIG_STRIDE应该满足可以被32位表示即可，最大可取`0x7fffffff`

### 参考答案对比

在`enqueue`中，我没有考虑time_slice已经被预先设置好的情况，已经按照参考答案修改为仅当time_slice为0或者大于max_time_slice时才初始化为max_time_slice。

我在enqueue时设置为0的priority为1，不应直接修改，应在pick_next时处理为0的情况。

均已修复。

## 总结

本实验中的课程知识点：

+ RR和Stride调度算法
+ 调度过程

本次实验没有涉及到的重要知识点：

+ 实时调度
+ 其他传统调度算法