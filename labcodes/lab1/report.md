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