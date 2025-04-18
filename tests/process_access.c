/*
 *  libpulp - User-space Livepatching Library
 *
 *  Copyright (C) 2021 SUSE Software Solutions GmbH
 *
 *  This file is part of libpulp.
 *
 *  libpulp is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  libpulp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libpulp.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

static char *banner = "Original banner";

/* Noinline must be set, else no calls are issued.  */
char *__attribute__((noinline)) banner_get(void) { return banner; }

/* Since clang-18 when compiling this function it is broken in two parts:
   banner_set and banner_set.specialized.1.  We don't want it on this test so
   we disable optimizations in this test.  */
#ifdef __clang__
#pragma clang optimize off
#endif
void __attribute__((noinline)) banner_set(char *new) { banner = new; }
#ifdef __clang__
#pragma clang optimize on
#endif

int
main(void)
{
  char buffer[128];

  /* Original banner. */
  printf("Banner addr: 0x%lX\n", (unsigned long)banner_get());
  printf("%s\n", banner_get());

  /* Use original banner setting function. */
  banner_set(strdup("Banner changed from main"));
  printf("%s\n", banner_get());

  /* Wait for input. */
  if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
    if (errno) {
      perror("access");
      return 1;
    }
  }

  /*
   * Use banner setting function again, which is supposed to have been
   * changed by the test driver. The patched function ignores the
   * argument, so 'String from main' should not be in the output.
   */
  banner_set("String from main");
  printf("%s\n", banner_get());

  return 0;
}
