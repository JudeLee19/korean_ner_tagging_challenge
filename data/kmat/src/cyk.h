#ifndef CYK_H
#define CYK_H

#include "ma.h"
#include "kmat.h"

extern int cyk_ExecM
(
	MAtRSC_M		*Rsc,			// [input] 형태소 단위 모델 리소스
	MAtRESULTS	*SubstrInfo,	// [input] 부분 문자열의 사전 정보
	int 			NumCell,		// [input] 삼각표의 셀의 수
	int			EjIndex,		// [input] 몇 번째 어절인가?
	char			**Substr,		// [input] 부분 문자열
	int			NumSyll,		// [input] 음절 수
	double		PhoneticProb,	// [input] 음운 복원 확률
	double		CutoffThreshold,	// [input] 컷오프 임계값
	double		AbsThreshold,	// [input] 절대 임계값
	MAtRESULTS	*Result		// [output] 결과 저장
);

#endif
