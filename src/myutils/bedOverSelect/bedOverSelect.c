/* bedOverSelect - my own bedIntersect adds 1,0 column if any row of B is hit by any A. */
#include "common.h"
#include "linefile.h"
#include "hash.h"
#include "options.h"
#include "binRange.h"

static char const rcsid[] = "$Id: newProg.c,v 1.28 2009/07/02 17:14:39 angie Exp $";

void usage()
/* Explain usage and exit. */
{
errAbort(
  "bedOverSelect - my own bedIntersect adds 1,0 column if any row of A is hit by any B\n"
  "usage:\n"
  "   bedIntersect a.bed b.bed output.bed\n"
  "options:\n"
  "   -aHitAny output all of a if any of it is hit by b\n"
  "   -aCoverage=0.N min coverage of b to output match.  Default .00001\n"
  "   -bScore output score from b.bed (must be at least 5 field bed)\n"
  );
}

//static boolean aHitAny = FALSE;
static boolean bScore = FALSE;
static float aCoverage = 0.00001;

static struct optionSpec optionSpecs[] = {
  //    {"aHitAny", OPTION_BOOLEAN},
    {"bScore", OPTION_BOOLEAN},
    {"aCoverage", OPTION_FLOAT},
    {NULL, 0}
};

struct bed3 
/* A three field bed. */
    {
    struct bed3 *next;
    char *chrom;	/* Allocated in hash. */
    int start;		/* Start (0 based) */
    int end;		/* End (non-inclusive) */
    };

struct bed5 
/* A five field bed. */
    {
    struct bed5 *next;
    char *chrom;	/* Allocated in hash. */
    int start;		/* Start (0 based) */
    int end;		/* End (non-inclusive) */
    char *name;	/* Name of item */
    int score; /* Score - 0-1000 */
    };

struct hash *readBed(char *fileName)
/* Read bed and return it as a hash keyed by chromName
 * with binKeeper values. */
{
char *row[3];
struct lineFile *lf = lineFileOpen(fileName, TRUE);
struct binKeeper *bk;
struct hash *hash = newHash(0);
struct hashEl *hel;
struct bed3 *bed;

while (lineFileRow(lf, row))
    {
    hel = hashLookup(hash, row[0]);
    if (hel == NULL)
       {
       bk = binKeeperNew(0, 1024*1024*1024);
       hel = hashAdd(hash, row[0], bk);
       }
    bk = hel->val;
    AllocVar(bed);
    bed->chrom = hel->name;
    bed->start = lineFileNeedNum(lf, row, 1);
    bed->end = lineFileNeedNum(lf, row, 2);
    if (bed->start > bed->end)
        errAbort("start after end line %d of %s", lf->lineIx, lf->fileName);
    binKeeperAdd(bk, bed->start, bed->end, bed);
    }
lineFileClose(&lf);
return hash;
}

struct hash *readBed5(char *fileName)
/* Read bed and return it as a hash keyed by chromName
 * with binKeeper values. */
{
char *row[5];
struct lineFile *lf = lineFileOpen(fileName, TRUE);
struct binKeeper *bk;
struct hash *hash = newHash(0);
struct hashEl *hel;
struct bed5 *bed;

while (lineFileRow(lf, row))
    {
    hel = hashLookup(hash, row[0]);
    if (hel == NULL)
       {
       bk = binKeeperNew(0, 511*1024*1024);
       hel = hashAdd(hash, row[0], bk);
       }
    bk = hel->val;
    AllocVar(bed);
    bed->chrom = hel->name;
    bed->start = lineFileNeedNum(lf, row, 1);
    bed->end = lineFileNeedNum(lf, row, 2);
    bed->score = lineFileNeedNum(lf, row, 4);
    if (bed->start > bed->end)
      errAbort("start after end line %d of %s", lf->lineIx, lf->fileName);
    binKeeperAdd(bk, bed->start, bed->end, bed);
    }
lineFileClose(&lf);
return hash;
}



void bedOverSelect(char *aFile, char *bFile, char *outFile)
/* bedOverSelect - my own bedIntersect adds 1,0 column if any row of A is hit by any B. */
{
struct hash *bHash = NULL;
struct lineFile *lf = lineFileOpen(aFile, TRUE);
FILE *f = mustOpen(outFile, "w");
char *row[40];
char *name;
float overlap;
int start, end, i, wordCount;
struct binElement *hitList = NULL, *hit;
struct binKeeper *bk;
struct bed5 *bed;

if (bScore)
    bHash = readBed5(bFile);
else
    bHash = readBed(bFile);
while ((wordCount = lineFileChop(lf, row)) != 0)
    {
    name = row[0];
    start = lineFileNeedNum(lf, row, 1);
    end = lineFileNeedNum(lf, row, 2);
    if (start > end)
        errAbort("start after end line %d of %s", lf->lineIx, lf->fileName);
    bk = hashFindVal(bHash, name);
    if (bk != NULL)
	{
	hitList = binKeeperFind(bk, start, end);
	if (hitList != NULL)
	  {
	    for (hit = hitList; hit != NULL; hit = hit->next)
	      {
		overlap = (float)positiveRangeIntersection(start, end, hit->start, hit->end);
		if (overlap/(float)(end-start) > aCoverage)
		  {
		    fprintf(f, "%s\t%d\t%d", name, start, end);
		    for (i = 3 ; i<wordCount; i++)
			  fprintf(f, "\t%s",row[i]);
		    if (bScore){
		      bed = hit->val;
		      fprintf(f, "\t%d",bed->score);
		    }
		    fprintf(f, "\t1\n");
		    break;
		  }
		else
		  {
		    //SKIP
		    //printf("filter out %s %d %d %d %d overlap %.1f %d %d %.3f\n",
		    //	   name,start,end, hit->start,hit->end, overlap, end-start, hit->end-hit->start,overlap/(float)(end-start));
		  }	
	      }
	  }
	else
	  {
	    fprintf(f, "%s\t%d\t%d", name, start, end);
	    for (i = 3 ; i<wordCount; i++)
	      fprintf(f, "\t%s",row[i]);
	    if (bScore)
	      fprintf(f, "\tNA");	    
	    fprintf(f, "\t0\n");
	  }
	slFreeList(&hitList);
	}
    }
}


int main(int argc, char *argv[])
/* Process command line. */
{
optionInit(&argc, argv, optionSpecs);

//aHitAny = optionExists("aHitAny");
bScore = optionExists("bScore");
aCoverage = optionFloat("aCoverage", aCoverage);
if (argc != 4)
    usage();
bedOverSelect(argv[1], argv[2], argv[3]);
return 0;
}

