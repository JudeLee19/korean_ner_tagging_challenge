#ifndef CYK_H
#define CYK_H

#include "ma.h"
#include "kmat.h"

extern int cyk_ExecM
(
	MAtRSC_M		*Rsc,			// [input] ���¼� ���� �� ���ҽ�
	MAtRESULTS	*SubstrInfo,	// [input] �κ� ���ڿ��� ���� ����
	int 			NumCell,		// [input] �ﰢǥ�� ���� ��
	int			EjIndex,		// [input] �� ��° �����ΰ�?
	char			**Substr,		// [input] �κ� ���ڿ�
	int			NumSyll,		// [input] ���� ��
	double		PhoneticProb,	// [input] ���� ���� Ȯ��
	double		CutoffThreshold,	// [input] �ƿ��� �Ӱ谪
	double		AbsThreshold,	// [input] ���� �Ӱ谪
	MAtRESULTS	*Result		// [output] ��� ����
);

#endif
