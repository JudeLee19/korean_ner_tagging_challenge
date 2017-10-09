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
	void		*Rsc,				// [input] ���ҽ�
	MAtRESULTS	*MorphAanlyzedResult,	// [input] ���¼� �м� ���
	int		NumWord,			// [input] ������ ��
	int		*StateSeq			// [output] �±� ��� (���¼� �м� ��� ��ȣ)
) ;

extern void tag_PrintResult
(
	FILE		*Fp,				// [output]
	char		**Words,			// [input] ���峻 ����
	MAtRESULTS	*MorphAanlyzedResult,	// [input] ���¼� �м� ���
	int		NumWord,			// [input] ������ ��
	int		*StateSeq			// [input] �±� ��� (���¼� �м� ����� ��ȣ)
);
			
#endif
