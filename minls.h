#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct part_entry {
   unsigned char bootind;      /* boot indicator 0/ACTIVE_FLAG   */
   unsigned char start_head;   /* head value for first sector    */
   unsigned char start_sec;    /* sector value + cyl bits for first sector */
   unsigned char start_cyl;    /* track value for first sector   */
   unsigned char sysind;       /* system indicator      */
   unsigned char last_head;    /* head value for last sector  */
   unsigned char last_sec;     /* sector value + cyl bits for last sector */
   unsigned char last_cyl;     /* track value for last sector    */
   unsigned long lowsec;       /* logical first sector     */
   unsigned long size;         /* size of partition in sectors   */
};

#define ACTIVE_FLAG  0x80  /* value for active in bootind field (hd0) */
#define NR_PARTITIONS   4  /* number of entries in partition table */
#define  PART_TABLE_OFF 0x1BE /* offset of partition table in boot sector */

/* Partition types. */
#define NO_PART      0x00  /* unused entry */
#define MINIX_PART   0x81  /* Minix partition type */


struct superblock { /* Minix Version 3 Superblock
                     * this structure found in fs/super.h
                     * in minix 3.1.1
                     */
   /* on disk. These fields and orientation are non–negotiable */

   uint32_t ninodes;       /* number of inodes in this filesystem */
   uint16_t pad1;          /* make things line up properly */
   int16_t i_blocks;       /* # of blocks used by inode bit map */
   int16_t z_blocks;       /* # of blocks used by zone bit map */
   uint16_t firstdata;     /* number of first data zone */
   int16_t log_zone_size;  /* log2 of blocks per zone */
   int16_t pad2;           /* make things line up again */
   uint32_t max_file;      /* maximum file size */
   uint32_t zones;         /* number of zones on disk */
   int16_t magic;          /* magic number */
   int16_t pad3;           /* make things line up again */
   uint16_t blocksize;     /* block size in bytes */
   uint8_t subversion;     /* filesystem sub–version */
};

#define DIRECT_ZONES 7

struct inode {
   uint16_t mode;    /* mode */
   uint16_t links;   /* number or links */
   uint16_t uid;
   uint16_t gid;
   uint32_t size;
   int32_t atime;
   int32_t mtime;
   int32_t ctime;
   uint32_t zone[DIRECT_ZONES];
   uint32_t indirect;
   uint32_t two_indirect;
   uint32_t unused;
};