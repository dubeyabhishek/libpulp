#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

__attribute__((noinline))
bool
repeat()
{
  volatile bool ret = true;
  return ret;
}

int
main()
{
  puts("Ready.");

  while (repeat())
    usleep(100);

  puts("Livepatched");
  fflush(stdout);

  pause();

  return 0;
}
