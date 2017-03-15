#include "minCommon.h"

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
   puts("here");
   printf("zone size when used: %d\n", zone_size);
   uint32_t dataSize = (((file.size - 1) / zone_size) + 1) * zone_size;
   puts("not here");
   data = nextData = malloc(dataSize);

   int zoneIdx = 0;

   while (nextData < data + file.size &&
          zoneIdx < DIRECT_ZONES) {
      fseek(image, file.zone[zoneIdx] * zone_size, SEEK_SET);
      fread(nextData, zone_size, 1, image);
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
      fseek(image, indirectZones[zoneIdx] * zone_size, SEEK_SET);
      fread(nextData, zone_size, 1, image);
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
         fseek(image, indirectZones[indirectZoneIdx] * zone_size, SEEK_SET);
         fread(nextData, zone_size, 1, image);
         nextData += zone_size;
         indirectZoneIdx++;
      }
      zoneIdx++;
   }

   return data;
}