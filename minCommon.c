#include "minCommon.h"

#define PARTITION_MSG \
"Partition %d out of range.  Must be 0..3.\n"

#define SUBPARTITION_MSG \
"Subpartition %d out of range.  Must be 0..3.\n"

#define SUB_INVALID "Not a Minix subpartition.\n"

#define PART_INVALID "Not a Minix partition.\n"

#define SB_MAG_MSG "Bad magic number. (0x%.4x)\n\
                    This doesn't look like a MINIX filesystem.\n"

#define USAGE_MSG \
   "usage: %s  [ -v ] [ -p num [ -s num ] ] imagefile [ path ]\n\
    Options:\n\
    \t-p\t part    --- select partition for filesystem (default: none)\n\
    \t-s\t sub     --- select subpartition for filesystem (default: none)\n\
    \t-h\t help    --- print usage information and exit\n\
    \t-v\t verbose --- increase verbosity level\n"

static uint32_t partitionOffset = 0;
static uint32_t partitionSize = -1;
char fullPathName[PATH_MAX] = "";

void parseArgs(int argc, char *const argv[], 
   struct minOptions *options, int whichProgram) {
   int opt;
   opterr = 0;

   while ((opt = getopt(argc, argv, "vp:s:")) != -1) {
      switch (opt) {
         case 'v':
            options->verbose++;
         break;

         case 'p':
            options->partition = atoi(optarg);
            if (options->partition < 0 || options->partition > 3) {
               fprintf(stderr, PARTITION_MSG, options->partition);
               fprintf(stderr, USAGE_MSG, argv[0]);
               exit(EXIT_FAILURE);
            }
         break;
         
         case 's':
            options->subpartition = atoi(optarg);
            if (options->subpartition < 0 || options->subpartition > 3) {
               fprintf(stderr, SUBPARTITION_MSG, options->subpartition);
               fprintf(stderr, USAGE_MSG, argv[0]);
               exit(EXIT_FAILURE);
            }
         break;

         default:
            fprintf(stderr, USAGE_MSG, argv[0]);
            exit(EXIT_FAILURE);
      }
   }
   if (optind < argc) {
      strcpy(options->imagefile, argv[optind]);
   }
   else {
      fprintf(stderr, USAGE_MSG, argv[0]);
   }
   optind++;
   if (optind < argc) {
      strcpy(options->path, argv[optind]);
      strcpy(options->fullPath, options->path);
   }
   else {
      strcpy(options->path, "/");
   }
   if (options->path[0] != '/') {
      char pathBase[PATH_MAX] = "/";
      strcat(pathBase, options->path);
      strcpy(options->path, pathBase);
   }
   strcpy(fullPathName, options->path);

   if (whichProgram == IS_MINGET) {
      optind++;
      if (optind < argc) {
         options->hasDestFile = HAS_DESTFILE;
         strcpy(options->destFile, argv[optind]);
      }
   }
}

void getMinixConfig(struct minOptions options, struct minixConfig *config) {
   // TODO: check for existing filename
   config->image = fopen(options.imagefile, "rb");

   if (options.partition >= 0) {
      setPartitionOffset(config->image, options.partition);
      if (options.subpartition >= 0) {
         setSubpartitionOffset(config->image, options.subpartition);
      }
   }

   /* Read the superblock */
   fseekPartition(config->image, 1024, SEEK_SET);

   fread(&(config->sb), sizeof(struct superblock), 1, config->image);

   // printSuperblock(sb);

   if (config->sb.magic != 0x4D5A) {
      fprintf(stderr, SB_MAG_MSG, config->sb.magic);
      exit(EXIT_FAILURE);
   }
   config->zone_size = config->sb.log_zone_size ? 
   (config->sb.blocksize << config->sb.log_zone_size) : config->sb.blocksize;
}

void setPartitionOffset(FILE *image, int partitionNum) {
   setOffset(image, partitionNum, 0);
}

void setSubpartitionOffset(FILE *image, int partitionNum) {
   setOffset(image, partitionNum, 1);
}

void setOffset(FILE *image, int partitionNum, int isSub) {
   /* Read the partition table */
   fseekPartition(image, 0x1BE, SEEK_SET);

   struct part_entry partition_table[4];
   fread(partition_table, sizeof(struct part_entry), 4, image);

   // for (i = 0; i < 4; i++) {
   //    printf("i: %d\n", i);
   //    printPartition(partition_table[i]);
   // }

   uint16_t *ptValid = malloc(sizeof(uint16_t));
   fread(ptValid, sizeof(uint16_t), 1, image);
   if (*ptValid != 0xAA55) {
      fprintf(stderr, "not a valid partition table (%X)\n", *ptValid);
      exit(EXIT_FAILURE);
   }

   struct part_entry *partition = partition_table + partitionNum;
   if (partition->sysind != 0x81) {
      fprintf(stderr, isSub ? SUB_INVALID : PART_INVALID);
      exit(EXIT_FAILURE);
   }
   partitionOffset = partition->lowsec * 512;
   partitionSize = partition->size;
}

/* 
 * Takes the root inode and an absolute path, and returns the inode 
 * of the requested file or directory.
 */
struct inode traversePath(struct inode *inodeTable, 
   uint32_t ninodes, char *path) {
   // printf("here\n");
   struct inode currnode = inodeTable[0];

   char *nextFile;
   char *file = strtok(path, "/");
   
   while (file) {
      nextFile = strtok(NULL, "/");

      if (!MIN_ISDIR(currnode.mode)) {
         fprintf(stderr, "not a directory\n");
         exit(EXIT_FAILURE);
      }
      int numFiles = currnode.size / sizeof(struct fileEntry);
      struct fileEntry *fileEntries;
      fileEntries = getFileEntries(currnode);

      struct fileEntry *currEntry = fileEntries;
      while (strcmp(currEntry->name, file) && 
             currEntry < fileEntries + numFiles) {
         currEntry++;
      }

      if (currEntry >= fileEntries + numFiles) {
         fprintf(stderr, "%s: File not found.\n", fullPathName);
         exit(EXIT_FAILURE);
      }
      currnode = *(struct inode *)getInode(currEntry->inode);

      file = nextFile;
   }

   return currnode;
}

struct fileEntry *getFileEntries(struct inode directory) {
   struct fileEntry *entries = (struct fileEntry *) copyZones(directory);
   return entries;
}

void *getInode(int inodeNum) {
   // printf("getting inode: %d\n", inodeNum);
   if (inodeNum == 0) {
      return NULL;
   }
   if (inodeNum > numInodes) {
      return NULL;
   }
   return &iTable[inodeNum - 1];
}

void *copyZones(struct inode file) {
   char *data, *nextData;
   // uint32_t dataSize = (((file.size - 1) / zone_size) + 1) * zone_size;
   uint32_t dataSize = file.size;
   data = nextData = malloc(dataSize);

   int zoneIdx = 0;

   while (nextData < data + file.size &&
          zoneIdx < DIRECT_ZONES) {
      uint32_t zoneNum = file.zone[zoneIdx];
      uint32_t readSize = nextData + zone_size > data + dataSize ?
                          data + dataSize - nextData : zone_size;
      if (zoneNum) {
         fseekPartition(image, zoneNum * zone_size, SEEK_SET);
         fread(nextData, readSize, 1, image);
      }
      else {
         memset(nextData, 0, readSize);
      }
      nextData += readSize;
      zoneIdx++;
   }

   if (nextData >= data + file.size) {
      return data;
   }


   int zoneNumsPerZone = zone_size / sizeof(uint32_t);

   uint32_t *indirectZones = malloc(sizeof(uint32_t) * zoneNumsPerZone);
   fseekPartition(image, file.indirect * zone_size, SEEK_SET);
   fread(indirectZones, sizeof(uint32_t), zoneNumsPerZone, image);
   zoneIdx = 0;

   while (nextData < data + file.size &&
          zoneIdx < zoneNumsPerZone) {
      uint32_t zoneNum = indirectZones[zoneIdx];
      uint32_t readSize = nextData + zone_size > data + dataSize ?
                          data + dataSize - nextData : zone_size;
      if (zoneNum) {
         fseekPartition(image, zoneNum * zone_size, SEEK_SET);
         fread(nextData, readSize, 1, image);
      }
      else {
         memset(nextData, 0, readSize);
      }
      nextData += readSize;
      zoneIdx++;
   }

   if (nextData >= data + file.size) {
      return data;
   }

   uint32_t *doubleIndirect = malloc(sizeof(uint32_t) * zoneNumsPerZone);
   fseekPartition(image, file.two_indirect * zone_size, SEEK_SET);
   fread(doubleIndirect, sizeof(uint32_t), zoneNumsPerZone, image);
   zoneIdx = 0;

   while (nextData < data + file.size &&
          zoneIdx < zoneNumsPerZone) {
      fseekPartition(image, doubleIndirect[zoneIdx] * zone_size, SEEK_SET);
      fread(indirectZones, sizeof(uint32_t), zoneNumsPerZone, image);

      int indirectZoneIdx = 0;

      while (nextData < data + file.size &&
             indirectZoneIdx < zoneNumsPerZone) {
         uint32_t zoneNum = indirectZones[indirectZoneIdx];
         uint32_t readSize = nextData + zone_size > data + dataSize ?
                             data + dataSize - nextData : zone_size;
         if (zoneNum) {
            fseekPartition(image, zoneNum * zone_size, SEEK_SET);
            fread(nextData, readSize, 1, image);
         }  
         else {
            memset(nextData, 0, readSize);
         }
         nextData += readSize;
         indirectZoneIdx++;
      }
      zoneIdx++;
   }

   return data;
}

size_t fseekPartition(FILE *stream, long int offset, int whence) {
   if (partitionSize > -1 && offset > partitionSize) {
      fprintf(stderr, "Attempting to seek outside of partition\n");
      exit(EXIT_FAILURE);
   }
   // fprintf(stderr, "seeking with partition: %d\n", partitionOffset);
   return fseek(stream, offset + partitionOffset, whence);
}