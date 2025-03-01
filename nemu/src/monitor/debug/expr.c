#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

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
          case TK_REG:
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
      (tokens[j-1].type>TK_REG||tokens[j-1].type==TK_EQ)){
        tokens[j].type=TK_MINUS;
      }
  }

  return true;
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  TODO();

  return 0;
}

