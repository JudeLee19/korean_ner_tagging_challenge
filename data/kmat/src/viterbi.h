#ifndef VITERBI_H
#define VITERBI_H

#include "ma.h"
#include "kmat_def.h"
#include "probtool.h"

extern void ViterbiSearch2gram
(
	PROBtFST		*IntraTrans,		// [input]
	PROBtFST		*InterTrans,		// [input]
	MAtRESULTS		*MorphAnalyzedResult,	// [input] 형태소 분석 결과 열
	int			NumWord,			// [input] 어절 수
	int			*StateSeq			// [output] 상태(형태소 분석 결과의 번호)열(결과 저장)
);

#endif
