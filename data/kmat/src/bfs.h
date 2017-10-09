#ifndef BFS_H
#define BFS_H

#include "kmat.h"
#include "ma.h"

// 너비 우선 탐색
extern void bigram_BFS
(
	MAtRSC_S		*Rsc,			// 리소스
	int			EjIndex,		// 어절 번호
	char			Syllable[][3],	// 어절 (음절열)
	int			WordLen,		// 어절 길이 (음절의 수)
	double		PhoneticProb,	// 음운 복원 확률
	double		CutoffThreshold,
	int			BeamSize,
	MAtRESULTS	*Result		// [output]결과 저장
);

#endif
