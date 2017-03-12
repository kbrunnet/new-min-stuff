#include "minls.h"
   
int main(int argc, char *const argv[])
{
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

   fseek(image, 1024, SEEK_SET);

   struct superblock sb;
   fread(&sb, sizeof(struct superblock), 1, image);

   // uint32_t buff[100];
   // memset(buff, 0, sizeof(buff));
   // fread(buff, sizeof(uint32_t), 5,image);
   // printf("%d\n", buff[0]);
   // fclose(image);


   // void *ptr = malloc(1024);
   // size_t bytesRead = fread(ptr, 1024, 1, image);
   // uint32_t * ptPtr = (uint32_t *)ptr;

   // printf("%X\n", ptr);
   // printf("%X\n", 0x1BE + (unsigned char *)ptr+1);

   // printf("%d\n", sizeof(uint32_t));
   // printf("%d\n", sizeof(0x1BE));

   // printf("magic number: %X\n", *( 0x1BE/32 + (unsigned char *)ptr));


   // struct part_entry *pPtr = (struct part_entry *) ptr;
   // printf("here\n");
   // printf("%X\n", pPtr->bootind);
   // printf("%X\n", pPtr->start_head);
   // printf("%X\n", pPtr->start_sec);
   // printf("%X\n", pPtr->start_cyl);
   // printf("%X\n", pPtr->sysind);
   // printf("%X\n", pPtr->last_head);
   // printf("%X\n", pPtr->last_sec);
   // printf("%X\n", pPtr->last_cyl);
   // printf("%X\n", pPtr->lowsec);
   // printf("%X\n", pPtr->size);

   // int i;
   // for (i = 0; i < 1000; i++) {
   // 	printf("%X: %X\n", ptr, *(unsigned char *)ptr);
   // 	ptr += 1;
   // }


   /* Other code omitted */

   exit(EXIT_SUCCESS);
}