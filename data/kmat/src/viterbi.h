#ifndef VITERBI_H
#define VITERBI_H

#include "ma.h"
#include "kmat_def.h"
#include "probtool.h"

extern void ViterbiSearch2gram
(
	PROBtFST		*IntraTrans,		// [input]
	PROBtFST		*InterTrans,		// [input]
	MAtRESULTS		*MorphAnalyzedResult,	// [input] ���¼� �м� ��� ��
	int			NumWord,			// [input] ���� ��
	int			*StateSeq			// [output] ����(���¼� �м� ����� ��ȣ)��(��� ����)
);

#endif
