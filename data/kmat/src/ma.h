#ifndef MA_H
#define MA_H

#include "kmat.h"
#include "kmat_def.h"
#include "probtool.h"
#include "phonetic_recover.h"

extern int ma_CompareProb
(
	const void *a,
	const void *b
);

///////////////////////////////////////////////////////////////////////////////
// ���ҽ� ����ü
typedef struct
{
	void	*fst;
	void	*info;
	void	*freq;
} MAtRSC_E;

typedef struct
{
	PROBtFST	*lexical;
	PROBtFST	*transition;
	PROBtFST	*morph;
} MAtRSC_M;

typedef struct
{
	PROBtFST	*tag_s;
	PROBtFST	*syllable_s;
	void		*s_transition_fst;
	void		*syllable_tag_fst;
	void		*syllable_tag_info;
} MAtRSC_S;

typedef struct
{
	MAtRSC_E		*e_rsc; // ���� ���� �м�
	PHONETICtRSC	*phonetic; // ���� ����
	MAtRSC_M		*m_rsc; // ���¼� ���� �м�
	MAtRSC_S		*s_rsc; // ���� ���� �м�
} MAtRSC;

///////////////////////////////////////////////////////////////////////////////
// ���� ���� �м�
extern MAtRSC_E *ma_OpenE
(
	char	*RMEJ_FST_Path,
	char	*RMEJ_FST_INFO_Path,
	char	*RMEJ_FST_FREQ_Path
);

extern void ma_CloseE
(
	MAtRSC_E *Rsc
);

extern int ma_ExecE
(
	MAtRSC_E	*Rsc,
	const char	*Eojeol,
	MAtRESULTS	*Result
);

///////////////////////////////////////////////////////////////////////////////
// ���¼� ���� �м�
extern MAtRSC_M *ma_OpenM
(
	char		*LEXICAL_FST_Path,
	char		*LEXICAL_INFO_Path,
	char		*LEXICAL_PROB_Path,
	char		*TRANSITION_FST_Path,
	char		*TRANSITION_PROB_Path,
	char		*M_MORPH_FST_Path,
	char		*M_MORPH_PROB_Path
);

extern void ma_CloseM
(
	MAtRSC_M	*Rsc
);

extern int ma_ExecM
(
	MAtRSC_M	*Rsc,
	MAtRESULTS	*RestoredEJ,
	int		IsFirst,	// ���� ���ۺ��ΰ�?
	int		IsLast,	// ���� ������ΰ�?
	MAtRESULTS	*Result, // [output]
	double	CutoffThreshold
);

///////////////////////////////////////////////////////////////////////////////
// ���� ���� �м�
extern MAtRSC_S *ma_OpenS
(
	char	*TAG_S_FST_Path,
	char	*TAG_S_PROB_Path, 
	char	*SYLLABLE_S_FST_Path,
	char	*SYLLABLE_S_PROB_Path,
	char	*S_TRANSITION_FST_Path,
	char	*SYLLABLE_TAG_FST_Path,
	char	*SYLLABLE_TAG_INFO_Path
);

extern void ma_CloseS
(
	MAtRSC_S *Rsc
);

extern int ma_ExecS
(
	MAtRSC_S	*Rsc,
	MAtRESULTS	*RestoredEJ, // ���� ������ ����
	MAtRESULTS	*Result, // [output]
	double	CutoffThreshold,
	int		BeamSize
);

#endif
