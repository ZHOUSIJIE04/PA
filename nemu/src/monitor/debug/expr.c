#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_HEX,
  TK_NUM,
  TK_REG,
  TK_AND,
  TK_NOT,
  TK_OR,
  TK_NEQ,
  TK_MINUS,
  TK_DER,//指针
  TK_PLUS,
  TK_MUL,
  TK_DIV,
  TK_LP,
  TK_RP,
  TK_SUB
  
  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {"0|[1-9][0-9]*",TK_NUM},
  {"0x[1-9A-Fa-f][0-9A-Fa-f]*",TK_HEX},
  {"\\$(eax|ecx|edx|ebx|esp|ebp|esi|edi|eip|ax|cx|dx|bx|sp|bp|si|di|al|cl|dl|bl|ah|ch|dh|bh)", TK_REG},
  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},         // plus
  {"\\-",TK_SUB},
  {"\\*",TK_MUL},
  {"\\/",TK_DIV},
  {"!=",TK_NEQ},
  {"\\(",TK_LP},
  {"\\)",TK_RP},
  {"&&",TK_AND},
  {"[\\|]{2}", TK_OR},
  {"==", TK_EQ}         // equal
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        tokens[nr_token].type=rules[i].token_type;
        switch (rules[i].token_type) {
          case TK_NOTYPE:
               break;
          case TK_NUM:
          case TK_HEX:
          case TK_REG:  //考虑是否需要去掉前面的$以及0x
               if(substr_len>32){
                printf("数字长度超过32位，无法正常识别！");
                break;
               }else{
                  strncpy(tokens[nr_token].str,substr_start,substr_len);
                  nr_token++;
                  break;
               }
          default:
             nr_token++;
             break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

int find_dominant_op(int p,int q){
  int op=0;
  int num=0;
  int minop=6;
  int temp=0;
  for(int i=p;i<=q;i++){
     if(tokens[i].type==TK_LP){
      num++;
      //printf("num=%d",num);
      continue;
     }else if(tokens[i].type==TK_RP){
      num--;
      continue;
     }else if(num==0){
       if(tokens[i].type==TK_NUM||tokens[i].type==TK_HEX||tokens[i].type==TK_REG||
          tokens[i].type==TK_DER||tokens[i].type==TK_MINUS){
          temp=0;
          continue;
        }else if(tokens[i].type==TK_OR){
          temp=1;
        }else if(tokens[i].type==TK_AND){
          temp=2;
        }else if(tokens[i].type==TK_EQ||tokens[i].type==TK_NEQ){
          temp=3; 
        }else if(tokens[i].type==TK_PLUS||tokens[i].type==TK_SUB){
          temp=4;
        }else if(tokens[i].type==TK_MUL||tokens[i].type==TK_DIV){
          temp=5;
        }
      if(temp<=minop){//必须取等号表示相同优先级下右边的那个更低
      minop=temp;
      op=i;
     }
     }
  }
  //printf("op=%d",op);
  return op;
}
uint32_t hex_to_dec(char str[32]) {
  uint result = 0;
  for (int i = 2; i < 10; ++i) {
      int tmp = 0;
      if (str[i] >= '0' && str[i] <= '9') {
          tmp = (int)str[i] - (int)'0';
      } else if (str[i] >= 'a' && str[i] <= 'f') {
          tmp = (int)str[i] - (int)'a' + 10;
      } else if (str[i] >= 'A' && str[i] <= 'F') {
          tmp = (int)str[i] - (int)'A' + 10;
      } else
          break;
      result = 16 * result + tmp;
  }
  return result;
}

bool check_parentheses(int left, int right){
  int num=0;
  for(int i=left;i<right;i++){
    if(tokens[i].type==TK_LP){
      num++;
    }else if(tokens[i].type==TK_RP){
      num--;
    }
  }
  if(num==0){
    return true;
  }else {
    return false;
  }
}

uint32_t eval(int p,int q ){
  if(p>q){
    printf("error: p > q\n");
    assert(0);
  }
  else if(p==q){
     if(tokens[p].type==TK_NUM){
      return atoi(tokens[p].str);
     }else if(tokens[p].type==TK_HEX){
      return hex_to_dec(tokens[p].str);
     }else if(tokens[p].type==TK_REG){
      char *str = tokens[p].str;
      if (str[0] == '$') {
          str++; // 跳过 '$' 符号
      }

      for (int i = 0; i < 8; i++) {
          if (strcmp(str, regsl[i]) == 0) {
              return reg_l(i);
          }
          if (strcmp(str, regsw[i]) == 0) {
              return reg_w(i);
          }
          if (strcmp(str, regsb[i]) == 0) {
              return reg_b(i);
          }
      }

      if (strcmp(str, "eip") == 0) {
          return cpu.eip;
      } else {
          printf("error in TK_REG in eval()\n");
          assert(0);
      }
     }

  }else if(p==q-1){ //单目运算符处理
    if (tokens[p].type == TK_NOT) {
      return !eval(q, q);
    } else if (tokens[p].type == TK_MINUS) {
        return -1 * eval(p + 1, q);
    }else if (tokens[p].type == TK_DER){
        uint32_t addr=eval(p+1,q);
        uint32_t result=vaddr_read(addr,4);
        return result;
    }
  }
  else if(tokens[p].type == TK_LP && tokens[q].type == TK_RP){
    //printf("kuohao");
    return eval(p+1,q-1);
  }

  else {
    int op = find_dominant_op(p,q);
    //printf("p=%d,q=%d",p,q);
    uint32_t val1=eval(p,op-1);
    uint32_t val2=eval(op+1,q);
    
    switch (tokens[op].type)
    {
      case TK_PLUS:
      return val1 + val2;
      case TK_SUB:
          return val1 - val2;
      case TK_MUL:
          return val1 * val2;
      case TK_DIV:
          return val1 / val2;
      case TK_EQ:
          return val1 == val2;
      case TK_NEQ:
          return val1 != val2;
      case TK_AND:
          return val1 && val2;
      case TK_OR:
          return val1 || val2;
      default:
          printf("Token computing fail!\n");
          assert(0);
  }
  }
  return 0;
}
uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  if(tokens[0].type==TK_MUL){
    tokens[0].type=TK_DER;
  }else if(tokens[0].type==TK_SUB){
    tokens[0].type=TK_MINUS;
  }

  for(int j=1;j<nr_token;j++){
      if(tokens[j].type==TK_MUL&&tokens[j-1].type!=TK_RP&&
        (tokens[j-1].type>TK_REG||tokens[j-1].type==TK_EQ))
          {
            tokens[j].type=TK_DER;
          }
      else if(tokens[j].type==TK_SUB&&tokens[j-1].type!=TK_RP&&
        (tokens[j-1].type>TK_REG||tokens[j-1].type==TK_EQ))
          {
            tokens[j].type=TK_MINUS;
          }
  }
  
  if (!check_parentheses(0, nr_token)) {
   *success = false;
   printf("括号不匹配！");
   return 0;
  } else {
    *success=true;
    return eval(0, nr_token - 1);
  }
}


