#include <stdio.h>
#include <unistd.h>

int main(int argc, char * argv[]) {
  int poor = 10;

  while (1) {
    printf("%i#(%i)[%p]", getpid(), poor, &poor);
    getchar();
    poor++;
  }

  return 0;
}
