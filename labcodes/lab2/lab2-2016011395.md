# 操作系统 lab2 Report

## 练习1：实现 first-fit 连续物理内存分配算法

原算法已经可以正常运行，只是原算法将原本生成的新块放在了链表头部，要实现FFMA只需对`default_alloc_pages`， `default_free_pages`两个函数进行修改即可。

### 分配

相对释放算法而言比较简单，只需要将新的小块放在原块后再删去原块即可。需要注意的是，应将新的小块`property`置位，否则新的小块无法被使用，修改部分的代码：

````C
    if (page != NULL) {
        if (page->property > n) {
            struct Page *p = page + n;
            p->property = page->property - n;
            list_add(&(page->page_link), &(p->page_link));	//将原空闲块剩余部分放在原块后
			SetPageProperty(p);	//将新块property置位
		}
        list_del(&(page->page_link));	//删去原块
        nr_free -= n;
        ClearPageProperty(page);
    }
````

### 释放

释放算法比较麻烦，但是实现起来其实很简单，难点在于容易漏想情况和`debug`不方便，在释放块时，我按如下情况进行了考虑：

+ 释放的块尾部恰好和另一块头部相接
+ 释放的块头部恰好和另一块尾部相接
+ 释放的块恰好嵌在某两块中间
+ 释放的块无法和其他块合并（最后实现时将如下两种子情况合并）
  + 原链表为空
  + 原链表不为空

实现过程中，也需要考虑`property`的置位，代码修改部分及解释：

````C
int flag = 1;	//用来表示当前情况，1表示没有和其他块连接
while (le != &free_list) {
		p = le2page(le, page_link);
		if (base + base->property == p) {	//若尾部与其他块头部相接
		    base->property += p->property;
            //头部和其他块相接，此时base已经在链表中，直接删去p并修改p的有效位
			if(flag == 0){
				list_del(&(p->page_link));
				ClearPageProperty(p);
			}
            //否则，base不在链表中，需要将原块放在p后一个，然后删去p，最后修改有效位即可
            else{
				list_add(&(p->page_link),&(base->page_link));
				list_del(&(p->page_link));
				SetPageProperty(base);
				ClearPageProperty(p);
			}
            flag = 0;
            break;	//不可能再和后面的块连接在一块，故可以终止
		}
    
    //若头部与其他块尾部相接，此时还需要继续扫描，因为尾部可能还有其他块相接（嵌在两块中间）
		else if (p + p->property == base) {	
            //修改p的长度以及p和base的有效位后，直接让base指向p
		    p->property += base->property;
			SetPageProperty(p);
			flag = 0;	//方便尾部还有相接块情况时判断。
			ClearPageProperty(base);
			base = p;
		}     
    	//若当前块已经大于base的尾部，无需继续遍历
    	else if(base + base->property < p){
            break;
        }
        le = list_next(le);
	}
	//若到最后还没将base加入链表，直接加入当前块的前一块即可；当前块只能是链表头（链表为空的情况）或者base后一块
	if(flag){
		list_add_before(le,&(base->page_link));
	}
    nr_free += n;
	}
````

### 可改进空间

+ 释放算法
  + 在释放时，应该不需要遍历整个链表，只需要计算出相邻块并直接判断状态即可

和答案相比，我的分配算法和答案一致，释放算法更加复杂，但只需要遍历一次链表中前一部分（地址小于base部分）即可，实际效率应该更高。