#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
  // int n = 0, i = 0, b = 0, p = 0, x = 0, e = 0;

  // printf("%d %d %d %d %d %d", n, i, b, p, x, e);
  char operand[10] = "#4096";
  // int a = 10;
  // char b = 'S';
  // printf("%X", strtol(operand, NULL, 10));
  // printf("%d", b);

  // for(int i = 2; i< 7; ++i){
  //   printf("%d \n", i);
  // }
  // char first_register[50], second_register[50];
  char* word;
  word = strtok(operand, "#");
  printf("1st word: %s\n", word);
  word = strtok(NULL, " \t\n");
  printf("2nd word: %s\n", word);
  // strcpy(second_register, word);
  word = strtok(NULL, " \t\n");
  printf("3rd word: %s\n", word);

  // int a = 0x00F00F5;
  // char object[10];
  // sprintf(object, "%X", a);
  // printf("%s", object);

  // printf("registers : %s %s ", first_register, second_register);

  return 0;
}