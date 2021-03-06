/* scanPwm - scans a sequence for motif matches to a PWM. */
#include "common.h"
#include "linefile.h"
#include "hash.h"
#include "options.h"
#include "dnautil.h"
#include "dnaseq.h"
#include "dnaLoad.h"

#include <omp.h>


static char const rcsid[] = "$Id: newProg.c,v 1.28 2009/07/02 17:14:39 angie Exp $";

void usage()
/* Explain usage and exit. */
{
errAbort(
  "scanPwm - scans a sequence for motif matches to a PWM\n"
  "usage:\n"
  "   scanPwm motif.pwm sequence\n"
  "where:\n"
  "   sequence is a .fa , .nib or .2bit file or a file which is a list of sequence files.\n"
  "options:\n"
  "   -j - use Jaspar *.pfm format (the default is Dan's TRANSFAC matrix)\n"
  "   -snp - discount most negative base in the pwm aligment if unkown snp hits the score \n"
  "   -t=6.91 - use threshold cut-off \n"
  "   -base=2.71828183  - use log in that base (default natural log) \n"
  "   -p=0.0 - add p pseudo-counts to frequencies (0.01 default to aboid -INF score) \n"
  "   -omp=1 - multi-core implementation (default 1 == one thread)"
  "   -mask - remove repeat masked bases \n"
  "   -xxx=XXX\n"
  );
}

static struct optionSpec options[] = {
   {"j", OPTION_BOOLEAN},
   {"mask", OPTION_BOOLEAN},
   {"snp", OPTION_BOOLEAN},
   {"t", OPTION_DOUBLE},
   {"p", OPTION_DOUBLE},
   {"base", OPTION_DOUBLE},
   {"omp" , OPTION_INT},
   {NULL, 0},
};

double addPseudoCounts = 0.01;
double thresh = 6.91 ;
double base = 2.71828183 ;
boolean useJaspar = TRUE;
boolean useSnpRobust =FALSE;
boolean maskRep = FALSE;
int ompNumThreads= 1;


#define maxPSSM 200
#define maxStr 500

struct pssm
{
  void *next;
  char *name;
  int w;
  double **matrix; /* */ 
};

void readInJasparPwm(struct pssm *pm, char *fileName);
void readInTransfacPwm(struct pssm *pm, char *fileName);
void allocateMemoryToMatrix (struct pssm *pwm);
void convertPSSMToLogs (struct pssm *pwm);
void reversePWM(struct pssm *op,const struct pssm *p);
void initialise_pssm(struct pssm *pm,char *fileName);
void printMatrix (struct pssm *pm);  
//void read_seq_fasta(struct parameters *p, struct sequence *s);
//void print_seq_fasta(FILE *f, struct sequence s);
int ATGCbase(char b);
double compare_subseq_to_pssm (char *seq, struct pssm *p);

//int get_pwm_scores(struct seq *seq, struct pssm *p);

void initialise_pssm(struct pssm *pwm,char *fileMotif) 
{
  int n,b,NumZeros=0;
  double FirstRowCounts=0.0,RowCounts=0.0;

  allocateMemoryToMatrix(pwm);
  if (useJaspar)
    readInJasparPwm(pwm,fileMotif);
  else
    readInTransfacPwm(pwm,fileMotif); 

  //Check if PWM is consistent and asses if probabilities or counts are provided. 
  for(n=0;n<pwm->w;n++){
    RowCounts=0.0;
    for(b=0;b<4;b++)
      RowCounts=RowCounts+pwm->matrix[n][b];
    for(b=0;b<4;b++)
      if(pwm->matrix[n][b]==0.0)
	NumZeros++;
    fprintf(stderr,"# Row %d: %g Counts\n",n,RowCounts);
    if(n==0)
      FirstRowCounts=RowCounts;
    //  else
      //assert(fabs(FirstRowCounts-RowCounts)<0.025);
      // if(fabs(FirstRowCounts-RowCounts)<0.025)
      //fprintf("#Row counts are not consistent across\n");
  }
  //fprintf(stderr, "Sum of first row %d\n",FirstRowCounts); 
  //Add pseudo_counts -- only possible if counts are provided...
  //Matrix is converted to probabilities to ...
  if(NumZeros>0)
    if(FirstRowCounts<1.1)
      addPseudoCounts=FirstRowCounts/100; 
  if(NumZeros==0)
    addPseudoCounts=0.0;  //Not necessary to add pseudo counts...

  // printMatrix(pwm);  
  fprintf(stderr,"#Adding pseudocounts %f\n",addPseudoCounts);
  for(n=0;n<pwm->w;n++){
    RowCounts=0.0;
    for(b=0;b<4;b++)
      pwm->matrix[n][b]=(pwm->matrix[n][b]+addPseudoCounts);
    for(b=0;b<4;b++)
      RowCounts=RowCounts+pwm->matrix[n][b];
    for(b=0;b<4;b++)
      pwm->matrix[n][b]=(pwm->matrix[n][b])/RowCounts;
  }
  printMatrix(pwm);  
  convertPSSMToLogs(pwm);
  printMatrix(pwm); 
}

void readInJasparPwm(struct pssm *pm, char *pwmFilePath)
{
  int row=0,col=0,num_rows=0;
  double c;
  FILE *inf;
  //  fprintf(stdout,"readInJasparPWM:\n");

  inf = mustOpen(pwmFilePath,"r");

  while(!feof(inf))
    {
    fscanf(inf,"%lf",&c);
    //   fprintf(stdout,"%d,",c);
    pm->matrix[row][col]=(double)c;    
    // fprintf(stdout,"%d,%f;",c,pm->matrix[row][col]);
    row++;
    if((c=getc(inf))=='\n')
      {      
        if(col==0)
	  num_rows=row;
	else
	  assert(num_rows==row); 
	
	row=0;
	col++;
	// fprintf(stdout,"--%d \n",col);
	if(col>3)
	  break;
      }
    else
      ungetc(c,inf);
    }

  fclose(inf);
  pm->w = num_rows;
  //p->pwmLength = num_rows;
  printMatrix(pm);  
}

void readInTransfacPwm(struct pssm *pm, char *pwmFilePath)
{
  int s,f=0,row=0,commentLine=0,nComLines=0,startPSSM=0;
  char str_buf,firstBit[maxStr];
  FILE *inf;

  inf = mustOpen(pwmFilePath,"r");

  while(!feof(inf)) {
    if(row >= maxPSSM) {
      fprintf(stderr,"PSSM too long (max is %d bp)\n",maxPSSM);
      exit(1);
    }
    if(startPSSM == 0) {
      s = fscanf(inf,"%c",&str_buf);
/*       printf("%c",str_buf); */
      if(s == EOF) break;
      if(commentLine) {
	if(str_buf == '\n') {
	  commentLine = 0;
	}
	continue;
      }
      if(str_buf == '#' || str_buf == 'A') { /* comment or header line  */
	commentLine++;
	nComLines++;
	continue;
      }
      startPSSM++;
    } 
    if(startPSSM > 0) {
      if(startPSSM==1) { /* just found the PSSM */
	firstBit[f] = str_buf; /* horrible hack to make sure we get the first digit on the first line */
	while((s = fscanf(inf,"%c",&str_buf))) {
	  if(str_buf == '\t' || str_buf == ' ') {
	    break;
	  }
	  f++;
	  firstBit[f] = str_buf;
	}
	firstBit[f+1] = '\0';
	pm->matrix[row][0] = atof(firstBit);
	s = fscanf(inf,"%lf %lf %lf",&pm->matrix[row][1],&pm->matrix[row][2],&pm->matrix[row][3]);
	startPSSM = 100;
      } else {
	s = fscanf(inf,"%lf %lf %lf %lf",&pm->matrix[row][0],&pm->matrix[row][1],&pm->matrix[row][2],&pm->matrix[row][3]);
	if(s == EOF) break;
      }
      row++;
    }
  }
  pm->w = row;
  //  p->pwmLength = row;
  fclose(inf);
}

void allocateMemoryToMatrix (struct pssm *p) {
  int i;
  p->matrix = malloc(maxPSSM*sizeof(double *));
 if(p->matrix == NULL) {
    fprintf(stderr,"Insufficient memory for p->matrix\n");
    exit(1);
  }
  for(i = 0;i < maxPSSM;i++) {
    p->matrix[i] = malloc(4*sizeof(double));
    if(p->matrix[i] == NULL) {
      fprintf(stderr,"Insufficient memory for p->matrix element %d\n",i);
      exit(1);
    }
    p->matrix[i][0] = 0.0;
  }
}

void convertPSSMToLogs (struct pssm *p) {
  int i,j;
  for(i=0;i<p->w;i++) {
    for(j=0;j<4;j++) {
      if(p->matrix[i][j]==0.0) { 
	fprintf(stderr,"Need to add pseudocounts to zero columns\n");
	exit(1);
      }
      p->matrix[i][j] = log(p->matrix[i][j]);
    }
  }
}

void reversePWM(struct pssm *op,const struct pssm *p){
  int i,I;
  double rowtmp[4];

  I=p->w;
  for(i=0;i<(I/2);i++) {
    rowtmp[0]=p->matrix[i][3];
    rowtmp[1]=p->matrix[i][2];
    rowtmp[2]=p->matrix[i][1];
    rowtmp[3]=p->matrix[i][0];
    op->matrix[i][0]=p->matrix[I-1-i][3];
    op->matrix[i][1]=p->matrix[I-1-i][2];
    op->matrix[i][2]=p->matrix[I-1-i][1];
    op->matrix[i][3]=p->matrix[I-1-i][0];
    op->matrix[I-1-i][0]=rowtmp[0];
    op->matrix[I-1-i][1]=rowtmp[1];
    op->matrix[I-1-i][2]=rowtmp[2];
    op->matrix[I-1-i][3]=rowtmp[3];
  }
  if((I%2)==1){ //if number of rows is odd we need to do this. 
    rowtmp[0]=p->matrix[i][3];
    rowtmp[1]=p->matrix[i][2];
    rowtmp[2]=p->matrix[i][1];
    rowtmp[3]=p->matrix[i][0];
    op->matrix[i][0]=rowtmp[0];
    op->matrix[i][1]=rowtmp[1];
    op->matrix[i][2]=rowtmp[2];
    op->matrix[i][3]=rowtmp[3];
  }    
}

void printMatrix (struct pssm *pm) {  
  int i;
  fprintf(stderr,"# N\tA\tC\tG\tT\n");
  for(i = 0;i < pm->w;i++) {
    fprintf(stderr,"# %d\t%1.3f\t%1.3f\t%1.3f\t%1.3f\n",i,pm->matrix[i][0],pm->matrix[i][1],pm->matrix[i][2],pm->matrix[i][3]);
  }
}

/// Returns 1 if non ATGC letter...
int ATGCbase(char b) {
  if(!maskRep){
    if(b=='A' || b=='a') {
      return 0;
    } else if(b=='C' || b=='c' ) {
      return 0;
    } else if(b=='G' || b=='g' ) {
      return 0;
    } else if(b=='T' || b=='t' ) {
      return 0;
    }
    return 1;
  }
  else{
    if(b=='A') {
      return 0;
    } else if(b=='C') {
      return 0;
    } else if(b=='G') {
      return 0;
    } else if(b=='T') {
      return 0;
    }
    return 1; 
  }
}

/// Compares subseq to PWM
double compare_subseq_to_pssm (char *seq, struct pssm *p) {
  int i;
  char b;
  double score=0,aux;
  double vmin=0;
  for(i=0;i<p->w;i++) {
    aux=0;
    b=seq[i];
    if(b == 'A'  || b=='a') {
      aux = p->matrix[i][0];
    } else if(b == 'C'  || b=='c') {
      aux = p->matrix[i][1];
    } else if(b == 'G'  || b=='g') {
      aux = p->matrix[i][2];
    } else if(b == 'T'  || b=='t') {
      aux = p->matrix[i][3];
    } else {
      aux = -100;
      /*       fprintf(stderr,"Skipping ambiguous character %c\n",seq[i]); */
    }
    if(aux<vmin)
      vmin=aux;
    score += aux;
  }
  /*   printf("%lf %lf\n",score,p.w*log(0.25)); */
  

  if(useSnpRobust)
    score = score - vmin - p->w*log(0.25);
  else
    score = score - p->w*log(0.25);

  return score/log(base);  // 
}



int scanPwmOneSeq(struct dnaSeq *seq,struct pssm *pwm)
{
  int i,j,Start,End,count;
  char nonATGCbase;
  double score;
  struct pssm revp,*rpwm;

  rpwm=&revp;
  allocateMemoryToMatrix(rpwm);
  rpwm->w=pwm->w;
  reversePWM(rpwm,pwm);
  
  count=-1;
  
  Start=0;
  End=seq->size-pwm->w+1;
  if((End-Start+1) < pwm->w)
    return 0; //Region too small to fit the motif..
#pragma omp parallel for private(nonATGCbase,j,score) reduction(+:count)
  for(i=Start;i<=End;i++){
    nonATGCbase=0;
    for(j=i;j<=(i+pwm->w-1);j++)
      if(ATGCbase(seq->dna[j])){
	nonATGCbase++;
	//shift i to skip??
      }
    if(nonATGCbase==0){
      score=compare_subseq_to_pssm(seq->dna+i,pwm); //Forward strand
      if(score >= thresh){
	fprintf(stdout,"%s\t%d\t%d\t+\t%f\n", seq->name,i,i+pwm->w-1,score);
	count++;
      }
      score=compare_subseq_to_pssm(seq->dna+i,rpwm);  //Reverse strand
      if(score >= thresh){
	fprintf(stdout,"%s\t%d\t%d\t-\t%f\n", seq->name,i,i+pwm->w-1,score);
	count++;
      }   	
    }  
  }

  return count;
}




/* ******************************************************************************** */

void scanPwm(char *fileMotif, char *fileSeq)
/* scanPwm - scans a sequence for motif matches to a PWM. */
{
  struct dnaLoad *dl = dnaLoadOpen(fileSeq);
  struct dnaSeq *seq; 

  struct pssm pwm;

  initialise_pssm(&pwm,fileMotif);
  
  //motifList = fixMotifs(dnaMotifLoadAll(motifFile));

  while ((seq = dnaLoadNext(dl)) != NULL)
    {
      verbose(2, "#\tprocessing: %s\n", seq->name);
      scanPwmOneSeq(seq,&pwm);   
    }
}

int main(int argc, char *argv[])
/* Process command line. */
{
optionInit(&argc, argv, options);
if (argc != 3)
    usage();

 dnaUtilOpen();

//motif = optionVal("motif", NULL);
//chr = optionVal("chr", NULL);
//strand = optionVal("strand", NULL);
 useJaspar = optionExists("j");
 useSnpRobust = optionExists("snp");
 maskRep = optionExists("mask");

 thresh= optionDouble("t",thresh);
 base= optionDouble("base",base);
 addPseudoCounts= optionDouble("p",addPseudoCounts);
 ompNumThreads= optionInt("omp",ompNumThreads);


 omp_set_num_threads(ompNumThreads); //Adjust this to env variable.... or option



// wigOutput = optionExists("wigOutput");

 scanPwm(argv[1],argv[2]);
return 0;
}
