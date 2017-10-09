#ifndef TAGGING_H
#define TAGGING_H

#include "probtool.h"
#include "kmat.h"

typedef struct
{
	PROBtFST *inter_transition;
	PROBtFST *intra_transition;
} TAGtRSC;

////////////////////////////////////////////////////////////////								
extern void *tag_Open
(
	char	*RSC_Path
);
	
extern void tag_Close
(
	void	*Rsc
);

extern int tag_Exec
(
	void		*Rsc,				// [input] 리소스
	MAtRESULTS	*MorphAanlyzedResult,	// [input] 형태소 분석 결과
	int		NumWord,			// [input] 어절의 수
	int		*StateSeq			// [output] 태깅 결과 (형태소 분석 결과 번호)
) ;

extern void tag_PrintResult
(
	FILE		*Fp,				// [output]
	char		**Words,			// [input] 문장내 어절
	MAtRESULTS	*MorphAanlyzedResult,	// [input] 형태소 분석 결과
	int		NumWord,			// [input] 어절의 수
	int		*StateSeq			// [input] 태깅 결과 (형태소 분석 결과의 번호)
);
			
#endif
