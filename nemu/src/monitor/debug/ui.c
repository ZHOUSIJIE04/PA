#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

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

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args){
   int N=0;
   if(args!=NULL){
      N=atoi(strtok(args," "));
      if(N<=0){
        printf("N must be greater than 0!");  
        return 0; 
      }
   }else{
      N=1;
   }
   cpu_exec(N);
   return 0;
}

static int cmd_p(char *args) {
  bool success = true;
  uint32_t res = expr(args, &success);

  if (success == false)
      printf("Expr calculation error!\n");
  else
      printf("Expr value = %d\n", res);

  return 0;
}


static int cmd_info(char * args){
  if(args ==NULL){
    printf("info后面必须有输入!");
    return 0;
  }
  if(strcmp(args,"r")==0)
  {
    for(int i = 0; i < 8; i++) {
      printf("%-8s0x%08x%16d\t", regsl[i], reg_l(i), reg_l(i));
      if(i%2==1){
        printf("\n");
      }
    }
    printf("%-8s0x%08x%16d\n", "eip", cpu.eip, cpu.eip);
    //16bit
    for(int i = 0; i < 8; i++) {
      printf("%-8s0x%08x%16d\t", regsw[i], reg_w(i), reg_w(i));
        if (i % 2 == 1)
            printf("\n");
    }
    //8bit
    for(int i = 0; i < 8; i++)
    {
      printf("%-8s0x%08x%16d\t", regsb[i], reg_b(i), reg_b(i));
        if (i % 2 == 1)
            printf("\n");
    }
    return 0;
  }
  else if(strcmp(args,"w")==0){
    //print_wp();
  }else{
    printf("输入的格式存在错误，请重新输入，或输入help获取提示！");
  }

  return 0;
}
static int cmd_x(char *args) {
  int cnt = 0;
  vaddr_t addr;
  int temp = sscanf(args, "%d 0x%x", &cnt, &addr);
  if(temp <= 0) {
    printf("args error\n");
    return 0;
  }
  printf("[Memory:]");
  for(int i = 0; i < cnt; i++) {
    if(i % 4 == 0) {
      printf("\n0x%x:  0x%02x", addr + i, vaddr_read(addr + i, 1));
    }  
    else {
      printf("  0x%02x", vaddr_read(addr + i, 1));
    }
  }
  printf("\n");
  return 0;
}

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  {"si", "arguments: [N]; carry out [N] instructions sequentially", cmd_si},
  {"info", "Print reg info or Print monitor point information", cmd_info},
  {"x", "Scan the memory", cmd_x},
  {"p", "Expr evaluation", cmd_p},

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

