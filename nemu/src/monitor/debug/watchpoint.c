#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;
static int wpnum=0;
void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
    wp_pool[i].hitTimes=0;
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;

}

/* TODO: Implement the functionality of watchpoint */

WP * new_wp(){
   if(free_==NULL){
    printf("no free wp !");
    assert(0);
   }
   WP * result=free_;
   free_=free_->next;
   result->next=NULL;
   if(head!=NULL)
   {
    result->next=head;
  }
   else
   {
    result->next=NULL;
   }
   head=result;
   result->NO=wpnum;
   wpnum++;
   return result;
}

void free_WP(WP * wp){
   wp->NO=0;
   wpnum--;
   if(free_!=NULL){
    wp->next=free_;
   }else{
    wp->next=NULL;
   }
   free_=wp;
}
bool delete_wp(int n){
  WP* temp=head;
  WP * cur=head->next;
  if(head->NO==n){
    head=head->next;
    free_WP(temp);
    return 1;
  }
  while(cur!=NULL){
     if(cur->NO==n){
      temp->next=cur->next;
      free_WP(cur);
      return 1;
     }else{
      temp=cur;
      cur=cur->next;
     }
  }
  return 0;
}

void print_wp() {
  if(head == NULL) {
    printf("no watchpoint now\n");
    return;
  }
  printf("watchpoint:\n");
  printf("NO.  expr    hitTimes\n");
  WP * wptemp = head;
  while (wptemp != NULL)
  {
    printf("%d  %s    %d\n", wptemp -> NO, wptemp -> expr, wptemp -> hitTimes);
    wptemp = wptemp ->next;
  }
}

bool check_WP(){
  if(head==NULL){
    return 0;
  }
  WP * temp=head;
  bool suc=false;
  bool changed=0;
  while (temp!=NULL)
  {
    uint32_t val=expr(temp->expr,&suc);
    if(val!=temp->result){
      changed = 1;
      temp->hitTimes++;
      printf("watchpoint No. %d has changed, old_value = %d, new_value = %d",temp->NO,temp->result,val);
      temp->result = val;
    }
    temp=temp->next;
  }
  return changed;
}
