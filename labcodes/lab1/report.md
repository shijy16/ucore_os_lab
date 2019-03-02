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

