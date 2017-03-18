/*
  TaiShell
  Copyright (C) 2017, Adrià Planas

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "taishell/tai_file.h"

uint32_t crc32(uint32_t crc, const void *buf, size_t size);

void usage(const char **argv) {
  fprintf(stderr, "usage: %s NAME VERSION MODE SCOPE outfile.tai \n", argv[0]);
  fprintf(stderr, "Parameters: \n");
  fprintf(stderr, " * NAME:\tName of the plugin. %d characters at most\n", MAX_PLUGIN_NAME);
  fprintf(stderr, " * VERSION:\tThe plugin version formatted as MM.mm \n");
  fprintf(stderr, " * MODE:\tKERNEL|USER\n");
  fprintf(stderr, " * SCOPE:\tSYSTEM|SHELL|APP\n");
  exit(1);
}

int main(int argc, const char **argv) {
  FILE *fout = NULL;

  tai_file_t tai = {
    .magic   = TAI_MAGIC_NUMBER,
    .version = TAI_FILE_VERSION
  };

  if (argc < 6)
    usage(argv);

  if(strlen(argv[1]) > MAX_PLUGIN_NAME) {
    fprintf(stderr, "Name exceeds the maximum length allowed (%d)\n", MAX_PLUGIN_NAME);
  }
  strncpy(tai.plugin.name, argv[1], MAX_PLUGIN_NAME);

  tai.plugin.id = crc32(0, tai.plugin.name, strlen(tai.plugin.name));

  unsigned int minor, major;
  if(sscanf(argv[2], "%u.%u", &major, &minor) < 2) {
    fprintf(stderr, "Invalid version \"%s\"\n", argv[2]);
    exit(1);
  }
  tai.plugin.version = major << 8 | minor;

  if(strcmp(argv[3], "KERNEL") == 0) {
    tai.plugin.flags = TSH_KERNEL;
  }
  else if(strcmp(argv[3], "USER") == 0) {
    tai.plugin.flags = TSH_USER;
  }
  else {
    fprintf(stderr, "Invalid mode \"%s\"\n", argv[3]);
    exit(1);
  }

  if(strcmp(argv[4], "SYSTEM") == 0) {
    tai.plugin.flags |= TSH_SYSTEM;
  }
  else if(strcmp(argv[4], "SHELL") == 0) {
    tai.plugin.flags |= TSH_SHELL;
  }
  else if(strcmp(argv[4], "APP") == 0) {
    tai.plugin.flags |= TSH_APP;
  }
  else {
    fprintf(stderr, "Invalid scope \"%s\"\n", argv[4]);
    exit(1);
  }

  fout = fopen(argv[5], "wb");
  if (!fout) {
    perror("Failed to open output file");
  }

  fwrite(&tai, sizeof(tai), 1, fout);
  fclose(fout);

  fprintf(stdout, "0x%08X", tai.plugin.id);
  exit(0);
}