/* bigWigInfo - Print out information about bigWig file.. */
#include "common.h"
#include "linefile.h"
#include "hash.h"
#include "options.h"
#include "localmem.h"
#include "bigWig.h"


static char const rcsid[] = "$Id: bigWigInfo.c,v 1.1 2009/06/29 17:50:50 kent Exp $";

void usage()
/* Explain usage and exit. */
{
errAbort(
  "bigWigInfo - Print out information about bigWig file.\n"
  "usage:\n"
  "   bigWigInfo file.bw\n"
  "options:\n"
  "   -xxx=XXX\n"
  );
}

static struct optionSpec options[] = {
   {NULL, 0},
};

void bigWigInfo(char *fileName)
/* bigWigInfo - Print out information about bigWig file.. */
{
struct bbiFile *bwf = bigWigFileOpen(fileName);
printf("version: %d\n", bwf->version);
printf("isSwapped: %d\n", bwf->isSwapped);
printf("zoomLevels: %d\n", bwf->zoomLevels);
struct bbiZoomLevel *zoom;
for (zoom = bwf->levelList; zoom != NULL; zoom = zoom->next)
    printf("\t%d\n", zoom->reductionLevel);
struct bbiChromInfo *chrom, *chromList = bbiChromList(bwf);
printf("chromCount: %d\n", slCount(chromList));
for (chrom=chromList; chrom != NULL; chrom = chrom->next)
    printf("\t%s %d %d\n", chrom->name, chrom->id, chrom->size);
}

int main(int argc, char *argv[])
/* Process command line. */
{
optionInit(&argc, argv, options);
if (argc != 2)
    usage();
bigWigInfo(argv[1]);
return 0;
}