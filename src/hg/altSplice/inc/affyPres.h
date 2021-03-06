/* affyPres.h was originally generated by the autoSql program, which also 
 * generated affyPres.c and affyPres.sql.  This header links the database and
 * the RAM representation of objects. */

#ifndef AFFYPRES_H
#define AFFYPRES_H

struct affyPres
/* Spread sheet data from Lily and Manny */
    {
    struct affyPres *next;  /* Next in singly linked list. */
    char *probeId;	/* Id of probe. */
    char *info;	/* Information associated with gene. */
    int valCount;	/* Number of experimental values. */
    float *vals;	/* Values from experiments. */
    int callCount;	/* Number of calls. */
    char **calls;	/* Calls for presence or not. */
    };

struct affyPres *affyPresLoad(char **row);
/* Load a affyPres from row fetched with select * from affyPres
 * from database.  Dispose of this with affyPresFree(). */

struct affyPres *affyPresLoadAll(char *fileName);
/* Load all affyPres from a tab-separated file.
 * Dispose of this with affyPresFreeList(). */

struct affyPres *affyPresCommaIn(char **pS, struct affyPres *ret);
/* Create a affyPres out of a comma separated string. 
 * This will fill in ret if non-null, otherwise will
 * return a new affyPres */

void affyPresFree(struct affyPres **pEl);
/* Free a single dynamically allocated affyPres such as created
 * with affyPresLoad(). */

void affyPresFreeList(struct affyPres **pList);
/* Free a list of dynamically allocated affyPres's */

void affyPresOutput(struct affyPres *el, FILE *f, char sep, char lastSep);
/* Print out affyPres.  Separate fields with sep. Follow last field with lastSep. */

#define affyPresTabOut(el,f) affyPresOutput(el,f,'\t','\n');
/* Print out affyPres as a line in a tab-separated file. */

#define affyPresCommaOut(el,f) affyPresOutput(el,f,',',',');
/* Print out affyPres as a comma separated list including final comma. */

/* -------------------------------- End autoSql Generated Code -------------------------------- */

#endif /* AFFYPRES_H */

