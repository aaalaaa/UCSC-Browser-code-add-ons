table knownGene
"Genes based on RefSeq, GenBank, and UniProt."
(
string  name;               "Name of gene"
string  chrom;              "Reference sequence chromosome or scaffold"
char[1] strand;             "+ or - for strand"
uint    txStart;            "Transcription start position"
uint    txEnd;              "Transcription end position"
uint    cdsStart;           "Coding region start"
uint    cdsEnd;             "Coding region end"
uint    exonCount;          "Number of exons"
uint[exonCount] exonStarts; "Exon start positions"
uint[exonCount] exonEnds;   "Exon end positions"
string  proteinID;          "UniProt display ID for Known Genes,  UniProt accession or RefSeq protein ID for UCSC Genes" 
string  alignID;            "Unique identifier for each (known gene, alignment position) pair"
)

