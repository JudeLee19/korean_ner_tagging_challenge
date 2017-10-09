#ifndef GET_SENTENCE_H
#define GET_SENTENCE_H

#include "kmat.h"

extern void FreeSentence
(
	char	**Words,
	int	NumWord
);

extern int GetSentence_Str
(
	char	*Str,
	char	**Words
);

extern int GetSentence_Row
(
	FILE	*Fp,
	char	**Words
);

extern int GetSentence_Column
(
	FILE	*Fp,
	char	**Words
);

extern int GetSentence_MA
(
	FILE		*Fp,
	char		**Words, 
	MAtRESULTS	*Result
);

#endif
