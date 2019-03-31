# LAB3 Report

石景宜 2016011395

## 练习1 给未被映射的地址映射上物理页

### 设计实现过程

根据提示，首先通过页目录基址和出错地址获得/创建对应页表项，若页表项对应页表不存在，则创建对应页。



#### 请描述页目录项（Page Directory Entry）和页表项（Page Table Entry）中组成部分对ucore实现页替换算法的潜在用处。

页目录项和页表项中对实现页替换算法有潜在用处的字段主要有：**访问位A**，**修改位D**，**存在位P**，其含义在LAB2中已经做过详细解释，不再赘述。

+ 访问位A主要可用于时钟替换算法和识别dirty bit的 extended clock页替换算法。
+ 修改位D，用于识别dirty bit的 extended clock页替换算法。
+ 存在位P，P为0,即当页面不存在于内存中时，其他位就可以用来存放任何所需要的信息，在本实验中用来存放了页的虚拟地址。



#### 如果ucore的缺页服务例程在执行过程中访问内存，出现了页访问异常，请问硬件要做哪些事情？

在执行过程中访问内存出现页访问异常时，硬件首先要将出错地址保存在CR2寄存器中，然后查找IDT，跳转到中断服务例程中，CPU会在当前栈中压入CS/EIP和`error code`，由于已经在内核态中，不会发生特权级切换，最后会把控制权交给中断服务例程。



#### 参考答案对比

对比后发现，我没有考虑获取页表项和分配页出错的情况，已修复。



## 练习二 补充完成基于FIFO的页面替换算法（需要编程）

### 设计实现过程

按照提示，在`vmm.c`的函数`do_page_fault`函数中，添加页换入过程代码，调用函数换入对应页后，建立映射关系，设置页为可交换，最后设置页的虚地址变量。

在`swap_fifo.c`中，完成对应两个函数，直接按照注释完成即可。

+ 在`_fifo_map_swappable`中，将新页插入到页表队列最后，即`head`之前
+ 在`_fifo_swap_out_victim`中，将最后访问的页从链表中删去并把对应页赋值给`*ptr_page`

### 如果要在ucore上实现"extended clock页替换算法"请给你的设计方案，现有的swap_manager框架是否足以支持在ucore中实现此算法？如果是，请给你的设计方案。如果不是，请给出你的新的扩展和基此扩展的设计方案。并需要回答如下问题

- 需要被换出的页的特征是什么？
- 在ucore中如何判断具有这样特征的页？
- 何时进行换入和换出操作？

##### 设计方案

现有框架可以实现此算法，设置一个全局页指针作为时钟指针，每次插入直接插入到head前即可，在换出时，需要进行判断访问位A，写入位D的取值对（A,D），进行不同的操作。

+ （0,0)，换出并让指针指向下一页。
+ （0,1)：将D位清零，为了保证一致性，需要将该页写入交换区，指针指向下一页继续查找。
+ （1,1），（1,0）：将A位置零，指针指向下一位继续查找。

##### 需要被换出的页的特征

在近期内既没有被访问又没有被修改。

##### ucore如何判断

通过访问位和修改位和设计方案所述操作判断。



##### 何时进行换入和换出操作

当前调用页不在页表中，且页表已满。

### 和参考答案对比

除了没有处理出错情况以外，`_fifo_swap_out`函数中，赋值语句放在了删除语句之前，应该不会有对正确性产生影响。



## 扩展练习 Challenge 1：实现识别dirty bit的 extended clock页替换算法（需要编程）

> 本练习代码位于`swap_clk.h`和`swap_clk.c`中，lab3_challenge分支中可直接`make qemu`运行。

### 设计

按照练习二中描述算法，在`swap_clk.h`中设置了一个全局链表指针`clk_ptr`。初始时置`clk_ptr`为`head->next`。

+ 页换入
  + 将换入页插入在时钟指针之前，保证该页最后被扫描。
+ 页换出
  + 按照练习二中描述实现即可，从指针处开始逐个判断和处理。直到找到(A,D)为(0,0)的页。

换出部分代码：

````c
	//指针是否需要初始化
	if(clk_ptr == NULL){
 		clk_ptr = head->next;
	}
	//跳过head
	if(clk_ptr == head){
		clk_ptr = clk_ptr->next;
	}
	//获得对应页表项
	struct Page* p_clk = le2page(clk_ptr,pra_page_link); 
	pte_t* pte_clk = get_pte(mm->pgdir,p_clk->pra_vaddr,0);
	//循环扫描
	while(1){
        //（1,x）
		if(*pte_clk&PTE_A){
			*pte_clk = *pte_clk&(~PTE_A);
			tlb_invalidate(mm->pgdir,p_clk->pra_vaddr);
		}else{
            //(0,1)
			if(*pte_clk&PTE_D){
				swapfs_write((p_clk->pra_vaddr/PGSIZE+1)<<8,p_clk);
				*pte_clk &= ~PTE_D;
			}else{	//(0,0)
				*ptr_page = p_clk;
				clk_ptr = clk_ptr->next;
				break;
			}
		}
        //指针指向下一个
		clk_ptr = clk_ptr->next;
		if(clk_ptr == head){
			clk_ptr = clk_ptr->next;
		} 
		p_clk = le2page(clk_ptr,pra_page_link);
		pte_clk = get_pte(mm->pgdir,p_clk->pra_vaddr,0);            
	}                                  
````

#### 测试

测试之前需要将内存中页进行一次初始化，将A,D位进行统一设置，否则不知道初始状态，无法进行测试。

已知内存中页虚拟地址为`0x1000,0x2000,0x3000,0x4000`,需要找到他们的页表项将A/D位置零，并将D位为1的页写入交换区，最后刷新tlb。

则初始时情况如下：

````c
a<- 0,0
b   0,0
c   0,0
d   0,0
````

测试流程如下：

写入e，应发生一次异常

```c
*(unsigned char*)0x5000 = 0x0e; 
assert(pgfault_num == 5);
e   0,1
b<- 0,0
c   0,0
d   0,0
```

读取e,不会发生异常

```c
cprintf("read e\n");
assert(*(unsigned char*)0x5000 == 0x0e);
assert(pgfault_num == 5);
e   1,1
b<- 0,0
c   0,0
d   0,0
```

读取a，发生一次异常

```c
cprintf("read a\n");
assert(*(unsigned char*)0x1000 == 0x0a);
assert(pgfault_num == 6);
e   1,1
a   1,0
c<- 0,0
d   0,0
```

读取c，不发生异常

```c
cprintf("read c\n");
assert(*(unsigned char*)0x3000 == 0x0c);
assert(pgfault_num == 6);
e   1,1
a   1,0
c<- 1,0
d   0,0
```

读取b，发生异常，应替换d

```c
cprintf("read b\n");
assert(*(unsigned char*)0x2000 == 0x0b);
assert(pgfault_num == 7);
e<- 1,1
a   1,0
c   0,0
b   1,0
```

写入d，并修改为0x0,c被替换

```c
cprintf("write d\n");
*(unsigned char*)0x4000 = 0x0; 
assert(pgfault_num == 8);
e   0,1
a   0,0
d   0,1
b<- 1,0
```

读取a，不发生异常

```c
cprintf("read a\n");
assert(*(unsigned char*)0x1000 == 0x0a);
assert(pgfault_num == 8);
e   0,1
a   1,0
d   0,1
b<- 1,0
```

读取c，替换b，发生异常

```c
cprintf("read c\n");
assert(*(unsigned char*)0x3000 == 0x0c);
assert(pgfault_num == 9);
e<- 0,0
a   0,0
d   0,0
c   1,0
```

读取e，a，不发生异常

````c
cprintf("read e,a\n");
assert(*(unsigned char*)0x5000 == 0x0e);
assert(*(unsigned char*)0x1000 == 0x0a);
assert(pgfault_num == 9);
e<- 1,0
a   1,0
d   0,0
c   1,0
````

读取b，d被替换

```c
cprintf("read b\n");
assert(*(unsigned char*)0x2000 == 0x0b);
assert(pgfault_num == 10);
e   0,0
a   0,0
b   1,0
c<- 1,0
```

最后,d应为0x0

````c
assert(*(unsigned char*)0x4000 == 0x0);
````

程序可以通过测试，因此程序应是正确实现了扩展时钟算法。



# 本实验中知识点

本实验中涉及的课程中知识点有：

- FIFO 页面替换算法
- 虚拟地址/物理地址映射关系建立
- 时钟替换算法
- 拓展时钟替换算法

还有没有涉及到的知识点有：

- 最优页面置换算法/LRU替换算法/LFU算法
- belady现象
- 全局置换算法
- 抖动问题和负载控制