#include <stdio.h>
#include <stdlib.h>
#include "stack.h"

// 用于模拟调用栈，主要用来记录函数里面是否有return语句，如果有，函数会马上中断，或许可以采用链表的方式来重写

int stack[MAXSIZE];
int top = -1;

int isempty() {
  if(top == -1)
    return 1;
  else
    return 0;
}

int isfull() {
  if(top == MAXSIZE)
    return 1;
  else
    return 0;
}

int peek() {
  return stack[top];
}

void set(int data) {
  stack[top] = data;
}

int pop() {
  int data;

  if(!isempty()) {
    data = stack[top];
    top = top - 1;
    return data;
  } else {
    printf("Could not retrieve data, Stack is empty.\n");
    abort();
  }
}

void push(int data) {
  if(!isfull()) {
    top = top + 1;
    stack[top] = data;
  } else {
    printf("Could not insert data, Stack is full.\n");
  }
}
