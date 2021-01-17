#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

typedef struct{
	swaddr_t prev_ebp;
	swaddr_t ret_addr;
	uint32_t args[4];

}PSF;


void cpu_exec(uint32_t);
void display_reg();
void get_func_from_addr(char *tmp,swaddr_t addr);
/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static void read_ebp(swaddr_t addr, PSF *ebp) {
	ebp->prev_ebp = swaddr_read(addr, 4,R_SS);
	ebp->ret_addr = swaddr_read(addr+4, 4,R_SS);
	int i;
	for ( i = 0; i < 4; i++)
	{
		ebp->args[i] = swaddr_read(addr+8+4*i, 4,R_SS);
	}
	
}

static int  cmd_bt(char * args){
	int a = 0 ;
	PSF current_ebp;
	char tmp[32];

	swaddr_t addr = reg_l(R_EBP);
	current_ebp.ret_addr = cpu.eip;
	while(addr>0){
		printf("#%d	0x%08x in\t",a++,current_ebp.ret_addr);

		get_func_from_addr(tmp,current_ebp.ret_addr);

		printf("%s\t",tmp);
		read_ebp(addr,&current_ebp);
		if(strcmp(tmp,"main")==0)printf("( )\n");
		else {
			printf("( %d, %d, %d, %d )\n", current_ebp.args[0], current_ebp.args[1], current_ebp.args[2], current_ebp.args[3]);
		}
		addr = current_ebp.prev_ebp;
	}
	return 0;
}

/* TODO: Add single step */
static int cmd_si(char *args) {
	char *arg = strtok(NULL, " ");
	int i = 1;

	if(arg != NULL) {
		sscanf(arg, "%d", &i);
	}
	cpu_exec(i);
	return 0;
}

/* TODO: Add info command */
static int cmd_info(char *args) {
	char *arg = strtok(NULL, " ");

	if(arg != NULL) {
		if(strcmp(arg, "r") == 0) {
			display_reg();
		}
		else if(strcmp(arg, "w") == 0) {
			list_watchpoint();
		}
	}
	return 0;
}

/* Add examine memory */
static int cmd_x(char *args) {
	char *arg = strtok(NULL, " ");
	int n;
	swaddr_t addr;
	int i;

	if(arg != NULL) {
		sscanf(arg, "%d", &n);

		bool success;
		addr = expr(arg + strlen(arg) + 1, &success);
		if(success) { 
			for(i = 0; i < n; i ++) {
				if(i % 4 == 0) {
					printf("0x%08x: ", addr);
				}

				printf("0x%08x ", swaddr_read(addr, 4,R_DS));
				addr += 4;
				if(i % 4 == 3) {
					printf("\n");
				}
			}
			printf("\n");
		}
		else { printf("Bad expression\n"); }

	}
	return 0;
}

/* Add expression evaluation  */
static int cmd_p(char *args) {
	bool success;

	if(args) {
		uint32_t r = expr(args, &success);
		if(success) { printf("0x%08x(%d)\n", r, r); }
		else { printf("Bad expression\n"); }
	}
	return 0;
}

/* Add set watchpoint  */
static int cmd_w(char *args) {
	if(args) {
		int NO = set_watchpoint(args);
		if(NO != -1) { printf("Set watchpoint #%d\n", NO); }
		else { printf("Bad expression\n"); }
	}
	return 0;
}

/* Add delete watchpoint */
static int cmd_d(char *args) {
	int NO;
	sscanf(args, "%d", &NO);
	if(!delete_watchpoint(NO)) {
		printf("Watchpoint #%d does not exist\n", NO);
	}

	return 0;
}

static int cmd_page(char *args) {
	if (args == NULL) return 0;
	lnaddr_t lnaddr;
	sscanf(args, "%x", &lnaddr);
	hwaddr_t hwaddr = page_translate(lnaddr, 1);
	if (!cpu.cr0.protect_enable || !cpu.cr0.paging)
	{
		printf("\033[1;33mPage address convertion is invalid.\n\033[0m");
	}
	printf("0x%x -> 0x%x\n", lnaddr, hwaddr);
	return 0;
	
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q }, 

	/* TODO: Add more commands */
        { "si", "Single step", cmd_si },
        { "info", "info r - print register values; info w - show watch point state", cmd_info },
	{ "x", "Examine memory", cmd_x },
        { "p", "Evaluate the value of expression", cmd_p },
	{ "w", "Set watchpoint", cmd_w },
	{ "d", "Delete watchpoint", cmd_d },
	{ "bt" , "Print stack frame",cmd_bt},
	{ "page", "Convert virtual address to physical address", cmd_page},
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
