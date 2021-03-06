/* functions for searching gene aliases */
#ifndef FINDKGALIAS_H
#define FINDKGALIAS_H

void addKgAlias(struct sqlConnection *conn, struct dyString *query,
       struct kgAlias **pList);
/* Query database and add returned kgAlias to head of list. */

struct kgAlias *findKGAlias(char *dataBase, char *spec, char *mode);
/* findKGAlias Looks up aliases for Known Genes, given a seach spec */
#endif
