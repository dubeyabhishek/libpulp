/*
 *  libpulp - User-space Livepatching Library
 *
 *  Copyright (C) 2019-2021 SUSE Software Solutions GmbH
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

#include <ctype.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <unistd.h>

#include "error_common.h"
#include "ulp_common.h"

/** @brief Get basename of a library in the path `name`
 *
 *  This functions get the basename of a library, stripping away any path that
 *  may come in the input string `name`.
 *
 *  Example:
 *
 *   ../libs/.lib/libpulp.so
 *
 *  This function will return:
 *
 *  libpulp.so
 *
 *  @param name Path to the library, or the name of the library itself.
 *  @return Basename of the library.
 */
const char *
get_basename(const char *name)
{
  const char *base = strrchr(name, '/');

  /* If strrchr returned non-null, it means that it found the last '/' in the
   * path, so add one to get the base name.  */
  if (base)
    return base + 1;

  return name;
}

/** @brief Convert build id provided in `build_id` into string.
 *
 * Example:
 *
 * with buildid: 338aa4d16c98dda7af170cc8e2b59d259bd5d4f4
 *
 * it will return the string:
 * "338aa4d16c98dda7af170cc8e2b59d259bd5d4f4"
 *
 * The string returned by this function is statically allocated and don't
 * require `free`.
 *
 * @param build_id The build id
 *
 * @return String representing buildid in hexadecimal format.
 */
const char *
buildid_to_string(const unsigned char build_id[BUILDID_LEN])
{
  static char build_id_str[2 * BUILDID_LEN + 1];
  int i;

  memset(build_id_str, '\0', sizeof(build_id_str));

  for (i = 0; i < BUILDID_LEN; i++)
    snprintf(&build_id_str[2 * i], 3, "%02x", (unsigned)build_id[i]);

  return build_id_str;
}

const char *
libpulp_strerror(ulp_error_t errnum)
{
  static const char *const libpulp_errlist[] = __ULP_ERRLIST;

  if (0xFF < errnum &&
      errnum < EUNKNOWN + (int)ARRAY_LENGTH(libpulp_errlist)) {
    return libpulp_errlist[errnum & 0xFF];
  }
  else {
    return strerror(errnum);
  }
}

/** @brief Get target program name
 *
 * For instance, assume that the program is named "binary", which the
 * user launched with "./binary".  This function will return the string
 * "binary".
 *
 * @return Target program binary's name.
 */
const char *
get_target_binary_name(int pid)
{
  static char binary_name[PATH_MAX];

  char fname[PATH_MAX];
  char cmdline[PATH_MAX];

  snprintf(fname, sizeof(fname), "/proc/%d/comm", pid);
  FILE *fp = fopen(fname, "r");
  if (fgets(cmdline, sizeof(cmdline), fp) != NULL) {
    strncpy(binary_name, get_basename(cmdline), PATH_MAX - 1);

    /* Remove any newlines from the string.  */
    for (int i = 0; i < (PATH_MAX - 1) && binary_name[i] != '\0'; i++) {
      if (binary_name[i] == '\n') {
        binary_name[i] = '\0';
        break;
      }
    }
  }
  fclose(fp);

  return binary_name;
}

/** @brief Get current program name
 *
 * For instance, assume that the program is named "binary", which the
 * user launched with "./binary".  This function will return the string
 * "binary".
 *
 * @return This program binary's name.
 */
const char *
get_current_binary_name()
{
  return get_target_binary_name(getpid());
}

/** @brief Check if string is actually a number.
 *
 * @param str  The string to check.
 *
 * @return     True if `str` is a number, False if not.
 */
bool
isnumber(const char *str)
{
  int i;

  if (str == NULL || *str == '\0')
    return false;

  for (i = 0; str[i] != '\0'; i++) {
    if (!isdigit(str[i]))
      return false;
  }

  return true;
}

/** @brief Creates a path to a temporary file.
 *
 * This function creates a path to a temporary file. The string returned is not
 * malloc'ed, so if you want to save the string somewhere you should `strdup`
 * it.
 *
 * @return Path to a temporary file.
 *
 */
const char *
create_path_to_tmp_file(void)
{
  const char *tmp_prefix = "/tmp/ulp-";
  static char buffer[24];
  FILE *f;

  /* Loop until we find an unused path.  If we are running multiple packer
     instances, we could eventually get a clash. */
  bool conflict = false;
  do {
    unsigned token;
    getrandom(&token, sizeof(unsigned), 0);
    snprintf(buffer, 24, "%s%u", tmp_prefix, token);
    f = fopen(buffer, "w");
    if (f == NULL)
      conflict = true;
  }
  while (conflict);

  fclose(f);

  return buffer;
}
