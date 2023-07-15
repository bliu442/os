1.编译os 有.o需要用到
2.进入user_program编译链接
3.将elf文件写入磁盘 .h文件需要加const修饰

---------------------------------------------------------------------------------------------
readelf -l user.elf

Elf 文件类型为 EXEC (可执行文件)
Entry point 0xb0010000
There are 5 program headers, starting at offset 52

程序头：
  Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align
  LOAD           0x000000 0xb000f000 0xb000f000 0x000d4 0x000d4 R   0x1000
  LOAD           0x001000 0xb0010000 0xb0010000 0x009e2 0x009e2 R E 0x1000
  LOAD           0x002000 0xb0011000 0xb0011000 0x00480 0x00480 R   0x1000
  LOAD           0x000000 0xb0030000 0xb0030000 0x00000 0x00404 RW  0x1000
  GNU_STACK      0x000000 0x00000000 0x00000000 0x00000 0x00000 RW  0x10

 Section to Segment mapping:
  段节...
   00     
   01     .text 
   02     .rodata .eh_frame 
   03     .bss 
   04     

---------------------------------------------------------------------------------------------
objdump -h user.elf

user.elf：     文件格式 elf32-i386

节：
Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         000009e2  b0010000  b0010000  00001000  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 .bss          00000404  b0030000  b0030000  00003000  2**5
                  ALLOC
  2 .rodata       00000130  b0011000  b0011000  00002000  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  3 .eh_frame     00000350  b0011130  b0011130  00002130  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  4 .comment      0000002d  00000000  00000000  00002480  2**0
                  CONTENTS, READONLY
  5 .debug_aranges 000000b8  00000000  00000000  000024ad  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
  6 .debug_info   00000a5c  00000000  00000000  00002565  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
  7 .debug_abbrev 000004a2  00000000  00000000  00002fc1  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
  8 .debug_line   000006a5  00000000  00000000  00003463  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
  9 .debug_str    0000022a  00000000  00000000  00003b08  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 10 .debug_line_str 0000012f  00000000  00000000  00003d32  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS

---------------------------------------------------------------------------------------------
size user.elf

   text    data     bss     dec     hex filename
   3682       0    1028    4710    1266 user.elf