#include "minls.h"

static unsigned int zone_size;
static struct inode *iTable;
static unsigned long firstDataAddress;
static FILE *image;

int main(int argc, char *const argv[])
{
   int i;
   int verbose = 0;
   int partition = -1;
   int subpartition = -1;
   char imagefile[NAME_MAX] = "";
   char path[PATH_MAX] = "";
   int opt;

   while ((opt = getopt(argc, argv, "vp:s:")) != -1) {
      switch (opt) {
         case 'v':
            verbose++;
         break;

         case 'p':
            partition = atoi(optarg);
         break;
         
         case 's':
            subpartition = atoi(optarg);
         break;

         default:
            fprintf(stderr, "Usage: minls [ -v ] [ -p part [ -s subpart ] ] imagefile [ path ]\n");
            exit(EXIT_FAILURE);
      }
   }

   if (optind < argc) {
      strcpy(imagefile, argv[optind]);
   }
   else {
      fprintf(stderr, "Usage: minls [ -v ] [ -p part [ -s subpart ] ] imagefile [ path ]\n");
   }
   optind++;
   if (optind < argc) {
      strcpy(path, argv[optind]);
   }
   else {
      strcpy(path, "/");
   }
   if (path[0] != '/') {
      char pathBase[PATH_MAX] = "";
      getcwd(pathBase, PATH_MAX);
      strcat(pathBase, "/");
      strcat(pathBase, path);
      strcpy(path, pathBase);
   }

   /*
   printf("verbose: %d\npartition: %d\nsubpartition:%d\nimagefile:%s\npath:%s\n",
            verbose,
            partition,
            subpartition,
            imagefile,
            path);
   */

   image = fopen(imagefile, "rb");

   /* Read the partition table */
   fseek(image, 0x1BE, SEEK_SET);

   struct part_entry partition_table[4];
   fread(partition_table, sizeof(struct part_entry), 4, image);

   // for (i = 0; i < 4; i++) {
   //    printf("i: %d\n", i);
   //    printPartition(partition_table[i]);
   // }

   /* TODO: f -p is set */
   uint16_t *ptValid = malloc(sizeof(uint16_t));
   fread(ptValid, sizeof(uint16_t), 1, image);
   if (*ptValid != 0xAA55) {
      printf("not a valid partition table\n");
   }

   /* Read the superblock */
   fseek(image, 1024, SEEK_SET);

   struct superblock sb;
   fread(&sb, sizeof(struct superblock), 1, image);

   // printSuperblock(sb);

   // if (sb.magic != 0x4D5A) {
   //    printf("invalid superblock, magic number must be 0x4D5A");
   //    // (EXIT_FAILURE);
   // }

   unsigned int zone_size = sb.log_zone_size ? 
      (sb.log_zone_size << 2) : sb.blocksize;
   firstDataAddress = sb.firstdata * zone_size;

   /* Read the root directory table */
   fseek(image, (2 + sb.i_blocks + sb.z_blocks) * sb.blocksize, SEEK_SET);
   struct inode iTable[sb.ninodes];
   fread(iTable, sizeof(struct inode), sb.ninodes, image);

   traversePath(iTable, sb.ninodes, path);
   
   exit(EXIT_SUCCESS);
}

/* 
 * Takes the root inode and an absolute path, and returns the inode 
 * of the requested file or directory.
 */
struct inode traversePath(struct inode *inodeTable, uint32_t ninodes, char *path) {
   struct inode currnode = inodeTable[0];

   char *file = strtok(path, "/");
   
   while (file) {

      int numFiles = currnode.size / sizeof(struct fileEntry);
      fseek(image, firstDataAddress, SEEK_SET);
      struct fileEntry fileEntries[numFiles];
      fread(fileEntries, sizeof(struct fileEntry), numFiles, image);
      printFiles(fileEntries, numFiles);

      struct fileEntry *currEntry = fileEntries;
      while (strcmp(currEntry->name, file) && 
             currEntry < fileEntries + numFiles) {
         currEntry++;
      }

      if (currEntry == fileEntries + numFiles) {
         fprintf(stderr, "File does not exist: %s\n", file);
         exit(EXIT_FAILURE);
      }
      
      printf("inode for file '%s':\n", file);
      printInode(currnode);

      printf("found file %s: see? %s\n", file, currEntry->name);
      currnode = inodeTable[currEntry->inode];

      file = strtok(NULL, "/");
   }

   return currnode;
}

void printFiles(struct fileEntry *fileEntries, int numFiles) {
   int i;
   for(i = 0; i < numFiles; i++) {
      printFile(&fileEntries[i]);
   }
}

void printFile(struct fileEntry *file) {
   printf("%d: ", file->inode);
   printf("%s\n", file->name);
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
   printf("  pad1: %d\n", sb.pad1);
   printf("  i_blocks: %d\n", sb.i_blocks);
   printf("  z_blocks: %d\n", sb.z_blocks);
   printf("  firstdata: %d\n", sb.firstdata);
   printf("  log_zone_size: %d\n", sb.log_zone_size);
   printf("  pad2: %d\n", sb.pad2);
   printf("  max_file: %u\n", sb.max_file);
   printf("  zones: %d\n", sb.zones);
   printf("  magic: 0x%x\n", sb.magic);
   printf("  pad3: 0x%x\n", sb.pad3);
   printf("  blocksize: %d\n", sb.blocksize);
   printf("  subversion: %d\n", sb.subversion);
}

void printInode(struct inode in) {
   int z;
   puts("inode: ");
   printf("  mode: 0x%x\n", in.mode);
   printf("  links: %u\n", in.links);
   printf("  uid: %u\n", in.uid);
   printf("  gid: %u\n", in.gid);
   printf("  size: %u\n", in.size);
   printf("  atime: %u\n", in.atime);
   printf("  mtime: %u\n", in.mtime);
   printf("  ctime: %u\n", in.ctime);
   printf("  Direct zones: \n");
   for (z = 0; z < DIRECT_ZONES; z++) {
      printf("\tzone[%u]\t=\t%u\n", z, in.zone[z]);
   }
   printf("  indirect: %u\n", in.indirect);
   printf("  double: %u\n", in.two_indirect);
}
