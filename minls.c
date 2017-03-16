#include "minls.h"

int main(int argc, char *const argv[])
{
   struct minOptions options;
   options.verbose = 0;
   options.partition = -1;
   options.subpartition = -1;
   options.imagefile = malloc(NAME_MAX);
   options.path = malloc(PATH_MAX);

   parseArgs(argc, argv, &options);
   strcpy(fileName, options.path);

   /*
   printf("verbose: %d\npartition: %d\nsubpartition:%d\nimagefile:%s\npath:%s\n",
            verbose,
            partition,
            subpartition,
            imagefile,
            path);
   */

   // TODO: check for existing filename
   image = fopen(options.imagefile, "rb");

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
      // printf("not a valid partition table\n");
   }

   /* Read the superblock */
   fseek(image, 1024, SEEK_SET);

   struct superblock sb;
   fread(&sb, sizeof(struct superblock), 1, image);

   // printSuperblock(sb);

   if (sb.magic != 0x4D5A) {
      fprintf(stderr, "Bad magic number. (0x%x)\nThis doesn't look like a MINIX filesystem.\n",
         sb.magic);
      exit(EXIT_FAILURE);
   }
   zone_size = sb.log_zone_size ? 
   (sb.blocksize << sb.log_zone_size) : sb.blocksize;
   printf("zone size when set: %d\n", zone_size);
   firstDataAddress = sb.firstdata * zone_size;

   numInodes = sb.ninodes;

   /* Read the root directory table */
   fseek(image, (2 + sb.i_blocks + sb.z_blocks) * sb.blocksize, SEEK_SET);

   iTable = (struct inode*) malloc(numInodes * sizeof(struct inode));
   fread(iTable, sizeof(struct inode), numInodes, image);
   // printf("numInode %d\n", numInodes);

   // printInodeFiles(iTable);

   // printInode(iTable[16]);
   // printInodeFiles(&iTable[16]);
   
   // printf("\n");
   // printf("\n");
   // printf("\n");
   struct inode destFile = traversePath(iTable, sb.ninodes, options.path);
   // printf("INODE RETURNED: \n");
   printInodeFiles(&destFile);

   exit(EXIT_SUCCESS);
}

void printInodeFiles(struct inode *in) {
   // printInode(*in);
   if (MIN_ISREG(in->mode)) {
      printPermissions(in->mode);
      printf("%10u ", in->size);
      printf("%s\n", fileName);
   }

   if (MIN_ISDIR(in->mode)) {
      struct fileEntry *fileEntries = getFileEntries(*in);
      int numFiles = in->size/sizeof(struct fileEntry);
      printf("numfiles: %d\n", numFiles);
      printFiles(fileEntries, numFiles);
   }
}

void printFiles(struct fileEntry *fileEntries, int numFiles) {
   int i;
   for(i = 0; i < numFiles; i++) {
      // printf("%u\n", fileEntries[i].inode);
      if (fileEntries[i].inode) {
         printFile(&fileEntries[i]);
      }
   }
}

void printFile(struct fileEntry *file) {
   // printf("%d ", file->inode);
   struct inode *iNode = (struct inode *) getInode(file->inode);
   if (iNode == NULL) {
      //if it gets here the inode is either
      //not stored inside our iTable 
      //OR, the inode was zero which is an
      //invalid inode
      // printf("null Inode\n");
   }
   else {   
      printPermissions(iNode->mode);
      printf("%10u ", iNode->size);
      printf("%s\n", file->name);
   }
}

void printPermissions(uint16_t mode) {
   printSinglePerm(MIN_ISDIR(mode), 'd');
   printSinglePerm(mode & MIN_IRUSR, 'r');
   printSinglePerm(mode & MIN_IWUSR, 'w');
   printSinglePerm(mode & MIN_IXUSR, 'x');
   printSinglePerm(mode & MIN_IRGRP, 'r');
   printSinglePerm(mode & MIN_IWGRP, 'w');
   printSinglePerm(mode & MIN_IXGRP, 'x');
   printSinglePerm(mode & MIN_IROTH, 'r');
   printSinglePerm(mode & MIN_IWOTH, 'w');
   printSinglePerm(mode & MIN_IXOTH, 'x');
}

void printSinglePerm(int print, char c) {
   if (print) {
      printf("%c", c);
   }
   else {
      printf("-");
   }
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
   printf("  %lX\n", partitionPtr.lowsec);
   printf("  %lX\n", partitionPtr.size);
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
