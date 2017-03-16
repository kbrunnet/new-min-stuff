#include "minCommon.h"

void parseArgs(int argc, char *const argv[], struct minOptions *options) {
   int opt;

   while ((opt = getopt(argc, argv, "vp:s:")) != -1) {
      switch (opt) {
         case 'v':
            options->verbose++;
         break;

         case 'p':
            options->partition = atoi(optarg);
         break;
         
         case 's':
            options->subpartition = atoi(optarg);
         break;

         default:
            fprintf(stderr, "Usage: minls [ -v ] [ -p part [ -s subpart ] ] imagefile [ path ]\n");
            exit(EXIT_FAILURE);
      }
   }
   if (optind < argc) {
      strcpy(options->imagefile, argv[optind]);
   }
   else {
      fprintf(stderr, "Usage: minls [ -v ] [ -p part [ -s subpart ] ] imagefile [ path ]\n");
   }
   optind++;
   if (optind < argc) {
      strcpy(options->path, argv[optind]);
   }
   else {
      strcpy(options->path, "/");
   }
   if (options->path[0] != '/') {
      char pathBase[PATH_MAX] = "/";
      strcat(pathBase, options->path);
      strcpy(options->path, pathBase);
   }
}

void getMinixConfig(struct minOptions options, struct minixConfig *config) {
   // TODO: check for existing filename
   config->image = fopen(options.imagefile, "rb");

   /* Read the partition table */
   fseek(config->image, 0x1BE, SEEK_SET);

   fread(config->partition_table, sizeof(struct part_entry), 4, config->image);

   // for (i = 0; i < 4; i++) {
   //    printf("i: %d\n", i);
   //    printPartition(partition_table[i]);
   // }

   /* TODO: f -p is set */
   uint16_t *ptValid = malloc(sizeof(uint16_t));
   fread(ptValid, sizeof(uint16_t), 1, config->image);
   if (*ptValid != 0xAA55) {
      // printf("not a valid partition table\n");
   }

   /* Read the superblock */
   fseek(config->image, 1024, SEEK_SET);

   fread(&(config->sb), sizeof(struct superblock), 1, config->image);

   // printSuperblock(sb);

   if (config->sb.magic != 0x4D5A) {
      fprintf(stderr, "Bad magic number. (0x%x)\nThis doesn't look like a MINIX filesystem.\n",
         config->sb.magic);
      exit(EXIT_FAILURE);
   }
   config->zone_size = config->sb.log_zone_size ? 
   (config->sb.blocksize << config->sb.log_zone_size) : config->sb.blocksize;
}

/* 
 * Takes the root inode and an absolute path, and returns the inode 
 * of the requested file or directory.
 */
struct inode traversePath(struct inode *inodeTable, uint32_t ninodes, char *path) {
   // printf("here\n");
   struct inode currnode = inodeTable[0];

   char *file = strtok(path, "/");
   
   while (file) {
      int numFiles = currnode.size / sizeof(struct fileEntry);
      struct fileEntry *fileEntries;
      fileEntries = getFileEntries(currnode);

      struct fileEntry *currEntry = fileEntries;
      while (strcmp(currEntry->name, file) && 
             currEntry < fileEntries + numFiles) {
         currEntry++;
      }

      if (currEntry >= fileEntries + numFiles) {
         fprintf(stderr, "File does not exist: %s\n", file);
         exit(EXIT_FAILURE);
      }
      currnode = *(struct inode *)getInode(currEntry->inode);

      file = strtok(NULL, "/");
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
   uint32_t dataSize = (((file.size - 1) / zone_size) + 1) * zone_size;
   data = nextData = malloc(dataSize);

   int zoneIdx = 0;

   while (nextData < data + file.size &&
          zoneIdx < DIRECT_ZONES) {
      uint32_t zoneNum = file.zone[zoneIdx];
      if (zoneNum) {
         fseek(image, zoneNum * zone_size, SEEK_SET);
         fread(nextData, zone_size, 1, image);
      }
      else {
         memset(nextData, 0, zone_size);
      }
      nextData += zone_size;
      zoneIdx++;
   }

   if (nextData >= data + file.size) {
      return data;
   }


   int zoneNumsPerZone = zone_size / sizeof(uint32_t);

   uint32_t *indirectZones = malloc(sizeof(uint32_t) * zoneNumsPerZone);
   fseek(image, file.indirect * zone_size, SEEK_SET);
   fread(indirectZones, sizeof(uint32_t), zoneNumsPerZone, image);
   zoneIdx = 0;

   while (nextData < data + file.size &&
          zoneIdx < zoneNumsPerZone) {
      uint32_t zoneNum = indirectZones[zoneIdx];
      if (zoneNum) {
         fseek(image, zoneNum * zone_size, SEEK_SET);
         fread(nextData, zone_size, 1, image);
      }
      else {
         memset(nextData, 0, zone_size);
      }
      nextData += zone_size;
      zoneIdx++;
   }

   if (nextData >= data + file.size) {
      return data;
   }

   uint32_t *doubleIndirect = malloc(sizeof(uint32_t) * zoneNumsPerZone);
   fseek(image, file.two_indirect * zone_size, SEEK_SET);
   fread(doubleIndirect, sizeof(uint32_t), zoneNumsPerZone, image);
   zoneIdx = 0;

   while (nextData < data + file.size &&
          zoneIdx < zoneNumsPerZone) {
      fseek(image, doubleIndirect[zoneIdx] * zone_size, SEEK_SET);
      fread(indirectZones, sizeof(uint32_t), zoneNumsPerZone, image);

      int indirectZoneIdx = 0;

      while (nextData < data + file.size &&
             indirectZoneIdx < zoneNumsPerZone) {
         uint32_t zoneNum = indirectZones[indirectZoneIdx];
         if (zoneNum) {
            fseek(image, zoneNum * zone_size, SEEK_SET);
            fread(nextData, zone_size, 1, image);
         }  
         else {
            memset(nextData, 0, zone_size);
         }
         nextData += zone_size;
         indirectZoneIdx++;
      }
      zoneIdx++;
   }

   return data;
}