#include "minls.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "partition.h"
#include "super.h"

void printPartition(struct part_entry  partitionPtr);

int main(int argc, char *const argv[])
{
   int i;
   int flags, opt;
   int nsecs, tfnd;

   nsecs = 0;
   tfnd = 0;
   flags = 0;
   while ((opt = getopt(argc, argv, "nt:")) != -1) {
      switch (opt) {
      case 'n':
         flags = 1;
         break;
      case 't':
         nsecs = atoi(optarg);
         tfnd = 1;
         break;
      default:

         fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
                 argv[0]);
         exit(EXIT_FAILURE);
      }
   }

   printf("flags=%d; tfnd=%d; optind=%d\n", flags, tfnd, optind);

   if (optind >= argc) {
      fprintf(stderr, "Expected argument after options\n");
      exit(EXIT_FAILURE);
   }

   printf("name argument = %s\n", argv[optind]);
   FILE *image = fopen(argv[optind], "rb");

   fseek(image, 0x1BE, SEEK_SET);

   struct part_entry partition_table[4];
   fread(partition_table, sizeof(struct part_entry), 4, image);

   for (i = 0; i < 4; i++) {
      printf("i: %d\n", i);
      printPartition(partition_table[i]);
   }

   uint16_t *ptValid = malloc(sizeof(uint16_t));
   fread(ptValid, sizeof(uint16_t), 1, image);
   if (*ptValid != 0xAA55) {
      printf("not a valid partition table\n");
   }

   fseek(image, 1024, SEEK_SET);

   struct superblock sb;
   fread(&sb, sizeof(struct superblock), 1, image);

   printSuperblock(sb);

   exit(EXIT_SUCCESS);
}

void printPartition(struct part_entry  partitionPtr) {
   printf("  %X\n", partitionPtr.bootind);
   printf("  %X\n", partitionPtr.start_head);
   printf("  %X\n", partitionPtr.start_sec);
   printf("  %X\n", partitionPtr.start_cyl);
   printf("  %X\n", partitionPtr.sysind);
   printf("  %X\n", partitionPtr.last_head);
   printf("  %X\n", partitionPtr.last_sec);
   printf("  %X\n", partitionPtr.last_cyl);
   printf("  %X\n", partitionPtr.lowsec);
   printf("  %X\n", partitionPtr.size);
}

void printSuperblock(struct superblock sb) {
   puts("SuperBlock: ");
   printf("  ninodes: %d\n", sb.ninodes);
   printf("  pad1: %X\n", sb.pad1);
   printf("  i_blocks: %d\n", sb.i_blocks);
   printf("  z_blocks: %d\n", sb.z_blocks);
   printf("  firstdata: %X\n", sb.firstdata);
   printf("  log_zone_size: %X\n", sb.log_zone_size);
   printf("  pad2: %X\n", sb.pad2);
   printf("  max_file: %X\n", sb.max_file);
   printf("  zones: %X\n", sb.zones);
   printf("  magic: %X\n", sb.magic);
   printf("  pad3: %X\n", sb.pad3);
   printf("  blocksize: %X\n", sb.blocksize);
   printf("  subversion: %X\n", sb.subversion);
}