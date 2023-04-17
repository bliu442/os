[bits 32]

;手动构建的堆栈
;|func_arg|
;|function|
;|.retaddr|
;|.....eip|
;|.....esi|
;|.....edi|
;|.....ebx|
;|.....ebp|
;线程第一次被调度使用上面的手动构建的堆栈 后续使用程序构建的堆栈
;第一次构建的esp固定(pcb-interrupt_stack_t-thread_stack_t) 后续esp不固定(线程中的局部变量/函数调用时压入的返回地址和参数,时钟中断触发时压入的寄存器(第一层上下文),switch_to函数压入的四个ABI(第二层上下文))
;后续程序构建的堆栈
;|........| 之前的局部变量等
;|..eflags| 进入中断时保存的上下文 第一层上下文
;|......cs|
;|.....eip| 在此处拿到发生中断时执行的函数,回到线程中
;|...error|
;|...index|
;|.....eax|
;|.....ecx|
;|.....edx|
;|.....ebx|
;|.....esp| popad忽略esp
;|.....ebp|
;|.....esi|
;|.....edi|
;|......ds|
;|......es|
;|......fs|
;|......gs|
;|.retaddr| 回到interrupt_handler_0x20
;|.retaddr| 回到clock_handler
;|....next|
;|.current|
;|.retaddr| 回到schedule
;|.....esi| 第二层上下文
;|.....edi|
;|.....ebx|
;|.....ebp|
section .text
global switch_to
switch_to:
	push esi ;在current的堆栈中保存他使用的寄存器 保存现场
	push edi
	push ebx
	push ebp

	mov eax, [esp + 20] ;拿到current pcb,current = task.stack
	mov [eax], esp ;保存esp到current的task.stack 实时堆栈得到保护

	mov eax, [esp + 24] ;拿到next pcb,next = task.stack
	mov esp, [eax] ;切换堆栈 切到next的堆栈中

;---------------------------------------------------------使用的堆栈已经不同了
	pop ebp ;从next的堆栈中弹出数据 恢复现场
	pop ebx
	pop edi
	pop esi

	xchg bx, bx
	
	ret ;后续的ret都是从next线程栈中拿到数据 实现了更改eip执行相应的线程函数

;第一次调度手动构建的堆栈
;1.thread_create将stack赋值为(pcb + PAGE_SIZE -interrupt_stack_t-thread_stack_t)
;2.switch_to将esp赋值为next->task.stack
;3.pop四个数据到相应寄存器,esp指向eip
;4.ret 将eip(kernel_thread)当作返回地址,传给eip寄存器,向下执行,执行线程函数
;目前esp指向retaddr 虽然kernel_thread是靠ret进入的,但执行时还是按照C语言规范 认为esp为返回地址 esp+4为第一个参数 esp+8为第二个参数
;5.取到参数function func_arg,开始执行线程函数

