#include "minls.h"

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

   fseek(image, 1024, SEEK_SET);

   struct superblock sb;
   fread(&sb, sizeof(struct superblock), 1, image);


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