#include <stdio.h>
#include <stdlib.h>
#include "terminal.h"

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  // TODO: Uncomment the code below to pass the first stage
  printf("$ ");
  terminal.accept_input();
  return 0;
}
