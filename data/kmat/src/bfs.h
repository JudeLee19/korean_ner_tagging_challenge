#ifndef BFS_H
#define BFS_H

#include "kmat.h"
#include "ma.h"

// �ʺ� �켱 Ž��
extern void bigram_BFS
(
	MAtRSC_S		*Rsc,			// ���ҽ�
	int			EjIndex,		// ���� ��ȣ
	char			Syllable[][3],	// ���� (������)
	int			WordLen,		// ���� ���� (������ ��)
	double		PhoneticProb,	// ���� ���� Ȯ��
	double		CutoffThreshold,
	int			BeamSize,
	MAtRESULTS	*Result		// [output]��� ����
);

#endif
