#include "minget.h"

char fullPath[PATH_MAX] = "";

int main(int argc, char *const argv[])
{
   struct minOptions options;
   options.verbose = 0;
   options.partition = -1;
   options.subpartition = -1;
   options.imagefile = malloc(NAME_MAX);
   options.path = malloc(PATH_MAX);
   options.fullPath = malloc(PATH_MAX);
   options.destFile = malloc(PATH_MAX);
   options.hasDestFile = 0;

   parseArgs(argc, argv, &options, IS_MINGET);
   strcpy(fullPath, options.fullPath);

   struct minixConfig config;
   config.image = NULL;

   getMinixConfig(options, &config);
   image = config.image;
   zone_size = config.zone_size;
   numInodes = config.sb.ninodes;

   /* Read the root directory table */
   fseekPartition(image, (2 + config.sb.i_blocks + config.sb.z_blocks) 
                         * config.sb.blocksize,
                  SEEK_SET);

   iTable = (struct inode*) malloc(numInodes * sizeof(struct inode));
   fread(iTable, sizeof(struct inode), numInodes, image);
   // printf("numInode %d\n", numInodes);

   // printInodeFiles(iTable);

   // printInode(iTable[16]);
   // printInodeFiles(&iTable[16]);
   
   // printf("\n");
   // printf("\n");
   // printf("\n");
   struct inode destFile = traversePath(iTable, 
   	config.sb.ninodes, options.path);
   // printf("INODE RETURNED: \n");
   // printInodeFiles(&destFile);

	char *data = copyZones(destFile);
	if (options.hasDestFile == HAS_DESTFILE) {
		FILE *writeToThisFile = fopen(options.destFile, "w");
   	if (writeToThisFile == NULL) {
   		printf("Can't write to this file!\n");
   	}
   	else {
		   if (MIN_ISREG(destFile.mode)) {
		   	fprintf(writeToThisFile, "%s", data);
		   }
			else {
				fprintf(writeToThisFile,
					"%s: Not a regular file\n", 
					fullPath);	
			}
   	}
	}
	else {
		if (MIN_ISREG(destFile.mode)) {
	   	fprintf(stdout, "%s", data);
	   }
		else {
			fprintf(stdout, 
				"%s: Not a regular file\n",
				fullPath);	
		}
	}

   exit(EXIT_SUCCESS);
}