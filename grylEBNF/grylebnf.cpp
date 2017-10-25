/*! Program converts EBNF-style grammar file to an easily-parseable GrylloBNF format.
 *
 *  GrylloBNF specs:
 *  - Bytes 0-3: Magic Number "gBNF"
 *  - Byte  4:   Version number
 *
 *  - Byte  5:   File property flags.
 *    - Bit 0: Tag table present
 *    - Bit 1: Grammar rule table present
 *
 *  - Byte  6-7: Tag table lenght in bytes
 *  - Byte  8-9: Grammar rule table lenght.
 *    (More size bytes can be present if more tables are present) 
 *
 *  - Remaining bytes - Data payload. Present in this order:
 *    1. Tag table
 *    2. Grammar rule table
 *    3. Additional tables.
 *
 *  - Tag table structure (n is variable, rows are terminated by \0).
 *    |   0   |   1   |   2   |  . . . . . . .  |  n-1  |   n   |
 *    [2-byte Tag ID]  [String representation of a tag]    [\0]  
 *
 *  - Grammar rule table structure:      
 *    |   0   |   1   |   2   |   3    |  4  | . . |  i  | . . .  |  j  | . . |  k  | k+1 |
 *    [2-byte Tag ID]  [No. of options]  [Option]   [\0]   . . .   [Option]    [\0]  [\0]
 *
 *    - [2-byte Tag ID]:  The ID of a tag this rule defines.
 *    - [No. of options]: The number of available definition options (in eBNF, separated by |).
 *    - [Option]:         gBNF-defined language option.
