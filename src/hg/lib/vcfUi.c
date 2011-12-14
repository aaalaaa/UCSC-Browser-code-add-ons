/* vcfUi - Variant Call Format user interface controls that are shared
 * between more than one CGI. */

#include "common.h"
#include "cheapcgi.h"
#include "errCatch.h"
#include "hCommon.h"
#include "hui.h"
#include "jsHelper.h"
#include "vcf.h"
#include "vcfUi.h"
#if (defined USE_TABIX && defined KNETFILE_HOOKS)
#include "knetUdc.h"
#include "udc.h"
#endif//def USE_TABIX && KNETFILE_HOOKS

INLINE char *nameOrDefault(char *thisName, char *defaultVal)
/* If thisName is not a placeholder value, return it; otherwise return default. */
{
if (isNotEmpty(thisName) && !sameString(thisName, "."))
    return thisName;
return defaultVal;
}

#define VCF_HAPLOSORT_DEFAULT_DESC "middle variant in viewing window"

static void vcfCfgHaplotypeCenterHiddens(char *track, char *ctrName, char *ctrChrom, int ctrPos)
/* Make hidden form inputs and button for setting the center variant for haplotype
 * clustering/sorting in hgTracks. */
{
char cartVar[1024];
safef(cartVar, sizeof(cartVar), "%s.centerVariantChrom", track);
cgiMakeHiddenVar(cartVar, ctrChrom);
safef(cartVar, sizeof(cartVar), "%s.centerVariantPos", track);
char ctrPosStr[16];
safef(ctrPosStr, sizeof(ctrPosStr), "%d", ctrPos);
cgiMakeHiddenVar(cartVar, ctrPosStr);
safef(cartVar, sizeof(cartVar), "%s.centerVariantName", track);
cgiMakeHiddenVar(cartVar, ctrName);
}

void vcfCfgHaplotypeCenter(struct cart *cart, struct trackDb *tdb, char *track,
			   boolean compositeLevel, struct vcfFile *vcff,
			   char *thisName, char *thisChrom, int thisPos, char *formName)
/* If vcff has genotype data, show status and controls for choosing the center variant
 * for haplotype clustering/sorting in hgTracks. */
{
if (vcff != NULL && vcff->genotypeCount > 1)
    {
    printf("<TABLE cellpadding=0><TR><TD>"
	   "<B>Haplotype sorting order:</B> using ");
    char *centerChrom = cartOptionalStringClosestToHome(cart, tdb, compositeLevel,
							"centerVariantChrom");
    if (isEmpty(centerChrom))
	{
	// Unspecified in cart -- describe the default action
	printf(VCF_HAPLOSORT_DEFAULT_DESC " as anchor.</TD></TR>\n");
	if (isNotEmpty(thisChrom))
	    {
	    // but we do have a candidate, so offer to make it the center:
	    puts("<TR><TD>");
	    vcfCfgHaplotypeCenterHiddens(track, thisName, thisChrom, thisPos);
	    char label[256];
	    safef(label, sizeof(label), "Use %s", nameOrDefault(thisName, "this variant"));
	    cgiMakeButton("submit", label);
	    printf(" as anchor</TD></TR>\n");
	    }
	else
	    printf("<TR><TD>To anchor the sorting to a particular variant, "
		   "click on the variant in the genome browser, "
		   "and then click on the 'Use this variant' button on the next page."
		   "</TD></TR>\n");
	}
    else
	{
	// Describe the one specified in cart.
	int centerPos = cartUsualIntClosestToHome(cart, tdb, compositeLevel, "centerVariantPos",
						  -1);
	char *centerName = cartStringClosestToHome(cart, tdb, compositeLevel, "centerVariantName");
	if (isNotEmpty(thisChrom))
	    {
	    // These form inputs are for either "use me" or clear:
	    vcfCfgHaplotypeCenterHiddens(track, thisName, thisChrom, thisPos);
	    // Is this variant the same as the center variant specified in cart?
	    if (sameString(thisChrom, centerChrom) && sameString(thisName, centerName) &&
		thisPos == centerPos)
		printf("this variant as anchor.</TD></TR>\n");
	    else
		{
		// make a "use me" button
		printf("%s at %s:%d as anchor.</TD></TR>\n<TR><TD>\n",
		       nameOrDefault(centerName, "variant"), centerChrom, centerPos+1);
		char label[256];
		safef(label, sizeof(label), "Use %s", nameOrDefault(thisName, "this variant"));
		cgiMakeButton("submit", label);
		printf(" as anchor</TD></TR>\n");
		}
	    }
	else
	    {
	    // Form inputs (in case the clear button is clicked)
	    vcfCfgHaplotypeCenterHiddens(track, centerName, centerChrom, centerPos);
	    printf("%s at %s:%d as anchor.</TD></TR>\n",
		   nameOrDefault(centerName, "variant"), centerChrom, centerPos+1);
	    }
	// Make a clear button that modifies the hiddens using onClick
	puts("<TR><TD>");
	struct dyString *onClick = dyStringNew(0);
	dyStringPrintf(onClick, "updateOrMakeNamedVariable(%s, '%s.centerVariantChrom', ''); ",
		       formName, track);
	dyStringPrintf(onClick, "updateOrMakeNamedVariable(%s, '%s.centerVariantName', ''); ",
		       formName, track);
	dyStringPrintf(onClick, "updateOrMakeNamedVariable(%s, '%s.centerVariantPos', 0);",
		       formName, track);
	dyStringPrintf(onClick, "document.%s.submit(); return false;", formName);
	cgiMakeButtonWithOnClick("submit", "Clear selection", NULL, onClick->string);
	printf(" (use " VCF_HAPLOSORT_DEFAULT_DESC ")</TD></TR>\n");
	}
    puts("</TABLE>");
    }
}

//TODO: share this code w/hgTracks, hgc in hg/lib/vcfFile.c
static struct vcfFile *vcfHopefullyOpenHeader(struct cart *cart, struct trackDb *tdb)
/* Defend against network errors and return the vcfFile object with header data, or NULL. */
{
#if (defined USE_TABIX && defined KNETFILE_HOOKS)
knetUdcInstall();
if (udcCacheTimeout() < 300)
    udcSetCacheTimeout(300);
#endif//def USE_TABIX && KNETFILE_HOOKS
char *db = cartString(cart, "db");
struct sqlConnection *conn = hAllocConnTrack(db, tdb);
char *fileOrUrl = bbiNameFromSettingOrTable(tdb, conn, tdb->table);
hFreeConn(&conn);
int vcfMaxErr = 100;
struct vcfFile *vcff = NULL;
/* protect against temporary network error */
struct errCatch *errCatch = errCatchNew();
if (errCatchStart(errCatch))
    {
    vcff = vcfTabixFileMayOpen(fileOrUrl, NULL, 0, 0, vcfMaxErr);
    }
errCatchEnd(errCatch);
if (errCatch->gotError)
    {
    if (isNotEmpty(errCatch->message->string))
	warn("unable to open %s: %s", fileOrUrl, errCatch->message->string);
    }
errCatchFree(&errCatch);
return vcff;
}

static void vcfCfgHapClusterEnable(struct cart *cart, struct trackDb *tdb, char *name,
				   boolean compositeLevel)
/* Let the user enable/disable haplotype sorting display. */
{
printf("<B>Enable Haplotype sorting display: </B>");
boolean hapClustEnabled = cartUsualBooleanClosestToHome(cart, tdb, compositeLevel,
							VCF_HAP_ENABLED_VAR, TRUE);
char cartVar[1024];
safef(cartVar, sizeof(cartVar), "%s." VCF_HAP_ENABLED_VAR, name);
cgiMakeCheckBox(cartVar, hapClustEnabled);
printf("<BR>\n");
}

static void vcfCfgHapClusterColor(struct cart *cart, struct trackDb *tdb, char *name,
				   boolean compositeLevel)
/* Let the user choose how to color the sorted haplotypes. */
{
printf("<B>Color sorted haplotypes by:</B>\n");
char *colorBy = cartUsualStringClosestToHome(cart, tdb, compositeLevel,
					     VCF_HAP_COLORBY_VAR, VCF_HAP_COLORBY_REFALT);
boolean colorByRefAlt = sameString(colorBy, VCF_HAP_COLORBY_REFALT);
char varName[1024];
safef(varName, sizeof(varName), "%s." VCF_HAP_COLORBY_VAR, name);
cgiMakeRadioButton(varName, VCF_HAP_COLORBY_REFALT, colorByRefAlt);
printf("reference/alternate alleles (reference = blue, alternate = red)\n");
cgiMakeRadioButton(varName, VCF_HAP_COLORBY_BASE, !colorByRefAlt);
printf("first base of allele (A = red, C = blue, G = green, T = magenta)<BR>\n");
}

static void vcfCfgHapClusterHeight(struct cart *cart, struct trackDb *tdb, struct vcfFile *vcff,
				   char *name, boolean compositeLevel)
/* Let the user specify a height for the track. */
{
if (vcff != NULL && vcff->genotypeCount > 1)
    {
    printf("<B>Haplotype sorting display height:</B> \n");
    int cartHeight = cartUsualIntClosestToHome(cart, tdb, compositeLevel,
					       VCF_HAP_HEIGHT_VAR, VCF_DEFAULT_HAP_HEIGHT);
    char varName[1024];
    safef(varName, sizeof(varName), "%s." VCF_HAP_HEIGHT_VAR, name);
    cgiMakeIntVarInRange(varName, cartHeight, "Height (in pixels) of track", 5, "10", "2500");
    puts("<BR>");
    }
}

static void vcfCfgHapCluster(struct cart *cart, struct trackDb *tdb, struct vcfFile *vcff,
			     char *name, boolean compositeLevel)
/* Show controls for haplotype-sorting display, which only makes sense to do when
 * the VCF file describes multiple genotypes. */
{
vcfCfgHapClusterEnable(cart, tdb, name, compositeLevel);
vcfCfgHaplotypeCenter(cart, tdb, name, compositeLevel, vcff, NULL, NULL, 0, "mainForm");
vcfCfgHapClusterColor(cart, tdb, name, compositeLevel);
vcfCfgHapClusterHeight(cart, tdb, vcff, name, compositeLevel);
//      thicken lines?
//      outline center variant?
}

static void vcfCfgMinQual(struct cart *cart, struct trackDb *tdb, struct vcfFile *vcff,
			  char *name, boolean compositeLevel)
/* If checkbox is checked, apply minimum value filter to QUAL column. */
{
char cartVar[1024];
safef(cartVar, sizeof(cartVar), "%s." VCF_APPLY_MIN_QUAL_VAR, name);
boolean applyFilter = cartUsualBooleanClosestToHome(cart, tdb, compositeLevel,
					VCF_APPLY_MIN_QUAL_VAR, VCF_DEFAULT_APPLY_MIN_QUAL);
cgiMakeCheckBox(cartVar, applyFilter);
printf("<B>Exclude items with QUAL score less than &nbsp;</B>\n");
double minQual = cartUsualDoubleClosestToHome(cart, tdb, compositeLevel, VCF_MIN_QUAL_VAR,
					      VCF_DEFAULT_MIN_QUAL);
safef(cartVar, sizeof(cartVar), "%s." VCF_MIN_QUAL_VAR, name);
cgiMakeDoubleVar(cartVar, minQual, 10);
printf("<BR>\n");
}

static void vcfCfgFilterColumn(struct cart *cart, struct trackDb *tdb, struct vcfFile *vcff,
			       char *name, boolean compositeLevel)
/* Show controls for filtering by value of VCF's FILTER column, which uses values defined
 * in the header. */
{
int filterCount = slCount(vcff->filterDefs);
if (filterCount < 1)
    return;
printf("<B>Exclude items with these FILTER values:</B><BR>\n");
char cartVar[1024];
safef(cartVar, sizeof(cartVar), "%s."VCF_EXCLUDE_FILTER_VAR, name);
jsMakeCheckboxGroupSetClearButton(cartVar, TRUE);
puts("&nbsp;");
jsMakeCheckboxGroupSetClearButton(cartVar, FALSE);
char *values[filterCount];
char *labels[filterCount];
int i;
struct vcfInfoDef *filt;
for (i=0, filt=vcff->filterDefs;  filt != NULL;  i++, filt = filt->next)
    {
    values[i] = filt->key;
    struct dyString *dy = dyStringNew(0);
    dyStringAppend(dy, filt->key);
    if (isNotEmpty(filt->description))
	dyStringPrintf(dy, " (%s)", filt->description);
    labels[i] = dyStringCannibalize(&dy);
    }
struct slName *selectedValues = NULL;
if (cartListVarExistsAnyLevel(cart, tdb, FALSE, VCF_EXCLUDE_FILTER_VAR))
    selectedValues = cartOptionalSlNameListClosestToHome(cart, tdb, FALSE, VCF_EXCLUDE_FILTER_VAR);
cgiMakeCheckboxGroupWithVals(cartVar, labels, values, filterCount, selectedValues, 1);
}

static void vcfCfgMinAlleleFreq(struct cart *cart, struct trackDb *tdb, struct vcfFile *vcff,
				char *name, boolean compositeLevel)
/* Show input for minimum allele frequency, if we can extract it from the VCF INFO column. */
{
printf("<B>Minimum minor allele frequency (if INFO column includes AF or AC+AN):</B>\n");
double cartMinFreq = cartUsualDoubleClosestToHome(cart, tdb, compositeLevel,
					   VCF_MIN_ALLELE_FREQ_VAR, VCF_DEFAULT_MIN_ALLELE_FREQ);
char varName[1024];
safef(varName, sizeof(varName), "%s." VCF_MIN_ALLELE_FREQ_VAR, name);
cgiMakeDoubleVarInRange(varName, cartMinFreq, "minor allele frequency between 0.0 and 0.5", 5,
			"0.0", "0.5");
puts("<BR>");
}

void vcfCfgUi(struct cart *cart, struct trackDb *tdb, char *name, char *title, boolean boxed)
/* VCF: Variant Call Format.  redmine #3710 */
{
boxed = cfgBeginBoxAndTitle(tdb, boxed, title);
printf("<TABLE%s><TR><TD>", boxed ? " width='100%'" : "");
struct vcfFile *vcff = vcfHopefullyOpenHeader(cart, tdb);
if (vcff != NULL)
    {
    boolean compositeLevel = isNameAtCompositeLevel(tdb, name);
    if (vcff->genotypeCount > 1)
	vcfCfgHapCluster(cart, tdb, vcff, name, compositeLevel);
    vcfCfgMinQual(cart, tdb, vcff, name, compositeLevel);
    vcfCfgFilterColumn(cart, tdb, vcff, name, compositeLevel);
    vcfCfgMinAlleleFreq(cart, tdb, vcff, name, compositeLevel);
    }
else
    {
    printf("Sorry, couldn't access VCF file.<BR>\n");
    }

puts("</TD>");
if (boxed && fileExists(hHelpFile("hgVcfTrackHelp")))
    printf("<TD style='text-align:right'><A HREF=\"../goldenPath/help/hgVcfTrackHelp.html\" "
           "TARGET=_BLANK>VCF configuration help</A></TD>");

printf("</TR></TABLE>");

if (!boxed && fileExists(hHelpFile("hgVcfTrackHelp")))
    printf("<P><A HREF=\"../goldenPath/help/hgVcfTrackHelp.html\" TARGET=_BLANK>VCF "
	   "configuration help</A></P>");
cfgEndBox(boxed);
}

