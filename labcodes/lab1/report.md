# Lab1

石景宜 2016011395

## 练习一

> 1.1操作系统镜像文件 ucore.img 是如何一步一步生成的?(需要比较详细地解释 Makefile中每一条相关命令和命令参数的含义,以及说明命令导致的结果)

```makefile
# create ucore.img
UCOREIMG	:= $(call totarget,ucore.img)

$(UCOREIMG): $(kernel) $(bootblock)
	$(V)dd if=/dev/zero of=$@ count=10000
	$(V)dd if=$(bootblock) of=$@ conv=notrunc
	$(V)dd if=$(kernel) of=$@ seek=1 conv=notrunc

$(call create_target,ucore.img)
```

表明`ucore.img`的生成需要`bootblock`,`kernel`两个文件,生成的命令语句如下：

- `dd if=/dev/zero of=bin/ucore.img count=10000`：其中`/dev/zero`为linux系统中提供无限个0的文件，此句代表用10000个初始为0的,大小为默认大小512字节的块填充`ucore.img`
- `dd if=bin/bootblock of=bin/ucore.img conv=notrunc`：将`bootblock`块拷贝到`ucore.img`的第一个块
- `dd if=bin/kernel of=bin/ucore.img seek=1 conv=notrunc`：将`kernel`块拷贝到`ucore.img`中第一个块之后

这三条命令导致最终生成的`ucore.img`为`bootblock`和`kernel`以及0组成

##### step 1. 生成`kernel`

生成`kernel`的`makefile`语句

````makefile
$(kernel): tools/kernel.ld

$(kernel): $(KOBJS)
	@echo + ld $@
	$(V)$(LD) $(LDFLAGS) -T tools/kernel.ld -o $@ $(KOBJS)
	@$(OBJDUMP) -S $@ > $(call asmfile,kernel)
	@$(OBJDUMP) -t $@ | $(SED) '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(call symfile,kernel)

$(call create_target,kernel)
````

实际命令：

````shell
+ ld bin/kernel
ld -m    elf_i386 -nostdlib -T tools/kernel.ld -o bin/kernel  obj/kern/init/init.o obj/kern/libs/stdio.o obj/kern/libs/readline.o obj/kern/debug/panic.o obj/kern/debug/kdebug.o obj/kern/debug/kmonitor.o obj/kern/driver/clock.o obj/kern/driver/console.o obj/kern/driver/picirq.o obj/kern/driver/intr.o obj/kern/trap/trap.o obj/kern/trap/vectors.o obj/kern/trap/trapentry.o obj/kern/mm/pmm.o  obj/libs/string.o obj/libs/printfmt.o

````

（`malefile`中后面几句对应命令没有找到）

表明`kernel`由一系列.o文件链接生成，它们均是操作系统的一些基础功能，且由如下`makefile`语句生成

````makefile
$(call add_files_cc,$(call listf_cc,$(KSRCDIR)),kernel,$(KCFLAGS))
````

表明这一系列.o文件均是由对应代码文件编译生成。

##### step 2. 生成`bootblock`

生成`bootblock`的makefile语句：

```makefile
$(bootblock): $(call toobj,$(bootfiles)) | $(call totarget,sign)
	@echo + ld $@
	$(V)$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 $^ -o $(call toobj,bootblock)
	@$(OBJDUMP) -S $(call objfile,bootblock) > $(call asmfile,bootblock)
	@$(OBJCOPY) -S -O binary $(call objfile,bootblock) $(call outfile,bootblock)
	@$(call totarget,sign) $(call outfile,bootblock) $(bootblock)

$(call create_target,bootblock)
```

表明`bootblock`的生成依赖于`sign`,`$(bootfiles)`,其中`$(bootfiles)`为`bootmain.o`和`bootasm.o`。

2.1 `$(bootfiles)`的生成

`makefile`语句如下：

````makefile
bootfiles = $(call listf_cc,boot)
$(foreach f,$(bootfiles),$(call cc_compile,$(f),$(CC),$(CFLAGS) -Os -nostdinc))
````

实际编译命令如下：

````shell
+ cc boot/bootasm.S
gcc -Iboot/ -fno-builtin -Wall -ggdb -m32 -gstabs -nostdinc  -fno-stack-protector -Ilibs/ -Os -nostdinc -c boot/bootasm.S -o obj/boot/bootasm.o
+ cc boot/bootmain.c
gcc -Iboot/ -fno-builtin -Wall -ggdb -m32 -gstabs -nostdinc  -fno-stack-protector -Ilibs/ -Os -nostdinc -c boot/bootmain.c -o obj/boot/bootmain.o
````

以上命令表明：`bootasm.o`由`bootasm.S`编译得到，`bootmain.o`由`bootmain.c`编译得到

2.2 `sign`的生成

`makefile`中语句如下：

````makefile
$(call add_files_host,tools/sign.c,sign,sign)
$(call create_target_host,sign,sign)
````

实际命令：

````shell
gcc -Itools/ -g -Wall -O2 -c tools/sign.c -o obj/sign/tools/sign.o
gcc -g -Wall -O2 obj/sign/tools/sign.o -o bin/sign
````

首先编译`sign.c`得到`sign.o`，然后由`sign.o`链接生成`sign`。

2.3 `bootblock`最终生成

生成`bootblock`的命令：

````shell
+ ld bin/bootblock
ld -m    elf_i386 -nostdlib -N -e start -Ttext 0x7C00 obj/boot/bootasm.o obj/boot/bootmain.o -o obj/bootblock.o
'obj/bootblock.out' size: 488 bytes
build 512 bytes boot sector: 'bin/bootblock' success!
````

表明`bootblock.o`由`bootasm.o`和`bootmain.o`链接得到。

没有发现makefile中以下语句对应命令

````makefile
	@$(OBJDUMP) -S $(call objfile,bootblock) > $(call asmfile,bootblock)
	@$(OBJCOPY) -S -O binary $(call objfile,bootblock) $(call outfile,bootblock)
	@$(call totarget,sign) $(call outfile,bootblock) $(bootblock)
````

查阅答案发现应还有命令（我的编译器并没有输出这几句）：

````shell
objcopy -S -O binary obj/bootblock.o obj/bootblock.out
bin/sign obj/bootblock.out bin/bootblock
````

可知，首先将`bootblock.o`的二进制代码拷贝到`bootblock.out`，然后用`sign`工具处理`bootblock.out`得到最后的`bootblock`。

##### step 3. `ucore.img`最终生成

- `dd if=/dev/zero of=bin/ucore.img count=10000`：其中`/dev/zero`为linux系统中提供无限个0的文件，此句代表用10000个初始为0的,大小为默认大小512字节的块填充`ucore.img`
- `dd if=bin/bootblock of=bin/ucore.img conv=notrunc`：将`bootblock`块拷贝到`ucore.img`的第一个块
- `dd if=bin/kernel of=bin/ucore.img seek=1 conv=notrunc`：将`kernel`块拷贝到`ucore.img`中第一个块之后

这三条命令导致最终生成的`ucore.img`为`bootblock`和`kernel`以及0组成



> 1.2 一个被系统认为是符合规范的硬盘主引导扇区的特征是什么？

因为sign将`bootblock`处理为了符合规范的硬盘主引导扇区，查看`sign.c`可知，符合规范的硬盘主引导扇区特征应该为大小为512字节，且最后两个字节为依次为：0x55，0xAA



## 练习二

> 2.1 从CPU加电后执行的第一条指令开始，单步跟踪BIOS的执行。

按照附录中提示配置`gdbinit`如下：

````shell
file bin/kernel
set architecture i8086
target remote :1234
break kern_init
define hook-stop
x /i $pc
end
````

使用`make debug`命令执行后，停止在第一条指令处，使用`where`指令查看其地址为`0xfff0`，执行`si`命令可以单步跟踪并在每一步输出当前指令。

运行结果：

````
(gdb) where 
#0  0x0000fff0 in ?? ()
(gdb) si
=> 0xe05b:      add    %al,(%eax)
0x0000e05b in ?? ()
(gdb) si
=> 0xe062:      add    %al,(%eax)
0x0000e062 in ?? ()
(gdb) c
Continuing.
=> 0x100000 <kern_init>:        push   %ebp

Breakpoint 1, kern_init () at kern/init/init.c:17
````



> 2. 2在初始化位置`0x7c00`设置实地址断点,测试断点正常。

修改`gdbinit`，将断点位置修改为`0x7c00`如下：

````shell
file bin/kernel
target remote :1234
set architecture i8086
break *0x7c00
define hook-stop
x /i $pc
end
````

经测试可以正常运行，运行结果如下：

````shell
0x0000fff0 in ?? ()
The target architecture is assumed to be i8086
Breakpoint 1 at 0x7c00
(gdb) c  
Continuing.
=> 0x7c00:      cli

Breakpoint 1, 0x00007c00 in ?? ()
````



> 2.3 从`0x7c00`开始跟踪代码运行,将单步跟踪反汇编得到的代码与`bootasm.S`和 `bootblock.asm`进行比较。

在2.2到达断点之后执行命令查看汇编代码如下：

````assembly
(gdb) x /20i $pc
=> 0x7c00:      cli
   0x7c01:      cld
   0x7c02:      xor    %eax,%eax
   0x7c04:      mov    %eax,%ds
   0x7c06:      mov    %eax,%es
   0x7c08:      mov    %eax,%ss
   0x7c0a:      in     $0x64,%al
   0x7c0c:      test   $0x2,%al
   0x7c0e:      jne    0x7c0a
   0x7c10:      mov    $0xd1,%al
   0x7c12:      out    %al,$0x64
   0x7c14:      in     $0x64,%al
   0x7c16:      test   $0x2,%al
   0x7c18:      jne    0x7c14
   0x7c1a:      mov    $0xdf,%al
   0x7c1c:      out    %al,$0x60
   0x7c1e:      lgdtl  (%esi)
   0x7c21:      insb   (%dx),%es:(%edi)
   0x7c22:      jl     0x7c33
   0x7c24:      and    %al,%al
````

经过对照，与`bootblock.asm`和`bootasm.s`一致。

> 2.4 自己找一个`bootloader`或内核中的代码位置，设置断点并进行测试。

找到`kernel.asm`如下代码：

````assembly
int
vcprintf(const char *fmt, va_list ap) {
  100229:	55                   	push   %ebp
  10022a:	89 e5                	mov    %esp,%ebp
  10022c:	83 ec 28             	sub    $0x28,%esp
    int cnt = 0;
  10022f:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
    vprintfmt((void*)cputch, &cnt, fmt, ap);
  100236:	8b 45 0c             	mov    0xc(%ebp),%eax
  100239:	89 44 24 0c          	mov    %eax,0xc(%esp)
  10023d:	8b 45 08             	mov    0x8(%ebp),%eax
  100240:	89 44 24 08          	mov    %eax,0x8(%esp)
  100244:	8d 45 f4             	lea    -0xc(%ebp),%eax
  100247:	89 44 24 04          	mov    %eax,0x4(%esp)
  10024b:	c7 04 24 09 02 10 00 	movl   $0x100209,(%esp)
  100252:	e8 0d 2b 00 00       	call   102d64 <vprintfmt>
    return cnt;
  100257:	8b 45 f4             	mov    -0xc(%ebp),%eax
}
````

在内核运行过程中，设置断点，查看位置`0x100229`的汇编代码如下：

````assembly
The target architecture is assumed to be i8086
Breakpoint 1 at 0x7c00
(gdb) b 0x100229
Function "0x100229" not defined.
Make breakpoint pending on future shared library load? (y or [n]) n
(gdb) b *0x100229
Breakpoint 2 at 0x100229: file kern/libs/stdio.c, line 27.
(gdb) c
Continuing.
=> 0x7c00:      cli

Breakpoint 1, 0x00007c00 in ?? ()
(gdb) c
Continuing.
=> 0x100229 <vcprintf>: push   %ebp

Breakpoint 2, vcprintf (fmt=0x10323c "%s\n\n", ap=0x7ba4 " 2\020") at kern/libs/stdio.c:27
(gdb) x /20i $pc
=> 0x100229 <vcprintf>: push   %ebp
   0x10022a <vcprintf+1>:       mov    %esp,%ebp
   0x10022c <vcprintf+3>:       sub    $0x28,%esp
   0x10022f <vcprintf+6>:       movl   $0x0,-0xc(%ebp)
   0x100236 <vcprintf+13>:      mov    0xc(%ebp),%eax
   0x100239 <vcprintf+16>:      mov    %eax,0xc(%esp)
   0x10023d <vcprintf+20>:      mov    0x8(%ebp),%eax
   0x100240 <vcprintf+23>:      mov    %eax,0x8(%esp)
   0x100244 <vcprintf+27>:      lea    -0xc(%ebp),%eax
   0x100247 <vcprintf+30>:      mov    %eax,0x4(%esp)
   0x10024b <vcprintf+34>:      movl   $0x100209,(%esp)
   0x100252 <vcprintf+41>:      call   0x102d64 <vprintfmt>
   0x100257 <vcprintf+46>:      mov    -0xc(%ebp),%eax
   0x10025a <vcprintf+49>:      leave
   0x10025b <vcprintf+50>:      ret
   0x10025c <cprintf>:  push   %ebp
   0x10025d <cprintf+1>:        mov    %esp,%ebp
   0x10025f <cprintf+3>:        sub    $0x28,%esp
   0x100262 <cprintf+6>:        lea    0xc(%ebp),%eax
   0x100265 <cprintf+9>:        mov    %eax,-0x10(%ebp)
````

对照可发现，二者一致。

## 练习三

> BIOS将通过读取硬盘主引导扇区到内存，并转跳到对应内存中的位置执行`bootloader`。请分析`bootloader`是如何完成从实模式进入保护模式的。

+ 清理寄存器

  + 屏蔽中断
  + 清理段寄存器

+ 开启`A20Gate`

  + 原因：早期8086CPU提供了20根地址线，可寻址1MB空间，但是`segment:offset`的表示能力超过了1MB，因此在超过时直接采用回卷的方案来解决这个问题。下一代80286的CPU提供了24位地址线，可以访问超过1MB的内容，但是为了向下兼容，使用了`A20Gate`来模拟回卷特征。但是在保护模式下要使用32位地址线，因此`A20Gate`在保护模式下都必须打开。

  + 打开步骤：

    + 读取8042控制器的`0x64`端口，判断`input buffer`是否为空，并等待直到其为空。
    + 向`0x64`端口写入写命令`0xdf`，再次执行上一步操作知道`input buffer`为空

    + 向`0x60`端口写入`0xdf`，将第二位置为`1`,意味着打开`A20Gate`

+ 加载GDT

  + 直接使用引导时建立的`GDT`，所以直接使用`lgdt gdtdesc`指令加载即可

+ 开启保护模式

  + 直接将`%cr0`寄存器的`PE`位置`1`

+ 长跳转到`32`位模式，更新代码段基地址

+ 初始化段寄存器（DS，GS，ES，FS，SS）

+ 初始化栈指针，建立堆栈

+ 进入启动主程序`bootmain`



## 练习四

> 通过阅读bootmain.c，了解bootloader如何加载ELF文件。通过分析源代码和通过qemu来运行并调试bootloader&OS，
>
> - bootloader如何读取硬盘扇区的？
> - bootloader是如何加载ELF格式的OS？
>
> 提示：可阅读“硬盘访问概述”，“ELF执行文件格式概述”这两小节。

#### 读取硬盘扇区

在`bootmain.c`中主要由

`readseg(uintptr_t va, uint32_t count, uint32_t offset)`函数以及`readsect(void *dst, uint32_t secno)`执行。

`readsect`函数执行过程：

````C
 37 static void		//等待磁盘空闲函数
 38 waitdisk(void) {
 39     while ((inb(0x1F7) & 0xC0) != 0x40);	//通过读取状态寄存器实现
 41 }   
 42     
 43 
 44 static void
 45 readsect(void *dst, uint32_t secno) {
 46    
 47     waitdisk();  //等待磁盘空闲
 48     
 49     outb(0x1F2, 1);                         // 读取一个扇区
 		
 		//secno指定扇区号和地址，0-27位是偏移地址，29-31位强制设为1，28位(=0)表示访问"Disk 0"
 50     outb(0x1F3, secno & 0xFF);
 51     outb(0x1F4, (secno >> 8) & 0xFF);
 52     outb(0x1F5, (secno >> 16) & 0xFF);
 53     outb(0x1F6, ((secno >> 24) & 0xF) | 0xE0);
 
 54     outb(0x1F7, 0x20);         //读磁盘命令
		//再次等待磁盘空闲
 57     waitdisk();
 58     
 59     // 从0x1F0读取数据到dst
 60     insl(0x1F0, dst, SECTSIZE / 4);
 61 }  
````

`readseg`对其进行了封装，实现了读取`count`字节数据的功能

````c
68 readseg(uintptr_t va, uint32_t count, uint32_t offset) {
 69     uintptr_t end_va = va + count;	//数据存储的结束位置
 70     
 71     // 为什么不end_va +=  offset % SECTSIZE ???
 72     va -= offset % SECTSIZE;
 73     
 74     // 从字节转换为扇区号，kernel扇区号1开始的
 75     uint32_t secno = (offset / SECTSIZE) + 1;
 76     
 77     // 循环读取
 80     for (; va < end_va; va += SECTSIZE, secno ++) {
 81         readsect((void *)va, secno);                                                                
 82     }
 83 }   

````

#### 加载ELF格式的OS

在`void boootmain()`函数中实现

````c
86 void
 87 bootmain(void) {
 88     // 读取硬盘中的elf头
 89     readseg((uintptr_t)ELFHDR, SECTSIZE * 8, 0);
 90     
 91     // 判断elf头是否合法
 92     if (ELFHDR->e_magic != ELF_MAGIC) {
 93         goto bad;
 94     }
 95     
 96     struct proghdr *ph, *eph; //程序表指针
 97     
 98     
 99     ph = (struct proghdr *)((uintptr_t)ELFHDR + ELFHDR->e_phoff);//ph表的地址
100     eph = ph + ELFHDR->e_phnum;	//ph表的结尾
		//将每个程序段读取到对应在内存中的虚拟地址中去
101     for (; ph < eph; ph ++) {
102         readseg(ph->p_va & 0xFFFFFF, ph->p_memsz, ph->p_offset);
103     }
104     
105     // 根据elf头部的入口信息，从程序入口启动内核
107     ((void (*)(void))(ELFHDR->e_entry & 0xFFFFFF))();
108     
109 bad:	//读取出错
110     outw(0x8A00, 0x8A00);                                                             
111     outw(0x8A00, 0x8E00);
112     
113     /* do nothing */
114     while (1);
115 }   

````

## 练习五

#### 实现过程

> 按照注释中一步一步实现

````c
305     uint32_t ebp = read_ebp(); 	//当前栈底位置
306     uint32_t eip = read_eip(); 	//当前pc
307     int i; 
308     for(i = 0;i < STACKFRAME_DEPTH;i++){	//循环打印整个调用栈信息，最多到栈的深度
309         cprintf("ebp:0x%08x eip:0x%08x ",ebp,eip); //当前ebp,eip
310         cprintf("args:"); 
311         uint32_t *args = (uint32_t *)ebp + 2; //当前ebp向上四个为可能的参数**注意在指针上+2相当于在地址上+8
312         cprintf("%08x %08x %08x %08x",args[0],args[1],args[2],args[3]); 
313         cprintf("\n"); 
314         print_debuginfo(eip - 1);//打印函数名等信息 
315         eip =  ((uint32_t *) ebp)[1]; 	//caller的eip在当前函数返回地址，即ebp上一个
316         ebp = ((uint32_t *) ebp)[0]; 	//caller的ebp就是当前ebp指向位置内数据
317         if(ebp == 0) break; 			//如果到了整个栈底，结束
318		}
````

#### 解释最后一行各个数值的含义

打印出来的最后一行为

```assembly
ebp:0x00007bf8 eip:0x00007d68 args:0xc031fcfa 0xc08ed88e 0x64e4d08e 0xfa7502a8
```

对应第一个被调用的函数`bootmain`，其`caller`的起始地址是`0x7c00`，栈底是`0x7bf8`，故`bootmain`的`ebp`是`0x7bf8`。`eip`是`bootmain`调用下一个函数返回后要执行的下一条指令地址，`args`是`bootmain`被调用的参数

## 练习六

> 1.中断描述符表（也可简称为保护模式下的中断向量表）中一个表项占多少字节？其中哪几位代表中断处理代码的入口？

8个字节，如下所示，第1-2字节和第7-8字节为偏移，第4-5字节为段选择子，他们一起构成了中断处理代码的入口。

>   offset      P DPL...        ss                    offset
>
> 31----16   15--------0   31------16         15-----0