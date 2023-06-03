#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "../stdint.h"
#include "./thread.h"

#define DIV_ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP))

typedef struct tss {
	uint32_t backlink;
	uint32_t esp0;
	uint32_t ss0;
	uint32_t esp1;
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint32_t trace;
	uint32_t io_base;
}__attribute__((packed)) tss_t;

extern uint32_t process_create_pdt(void);
extern void process_pdt_activate(task_t *process);
extern void user_bucket_dir_init(task_t *process);
extern void process_start(void *filename, char *name);
extern void process_activate(task_t *process);

#endif