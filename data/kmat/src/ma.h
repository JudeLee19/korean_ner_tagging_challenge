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
// 리소스 구조체
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
	MAtRSC_E		*e_rsc; // 어절 단위 분석
	PHONETICtRSC	*phonetic; // 음운 복원
	MAtRSC_M		*m_rsc; // 형태소 단위 분석
	MAtRSC_S		*s_rsc; // 음절 단위 분석
} MAtRSC;

///////////////////////////////////////////////////////////////////////////////
// 어절 단위 분석
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
// 형태소 단위 분석
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
	int		IsFirst,	// 어절 시작부인가?
	int		IsLast,	// 어절 종료부인가?
	MAtRESULTS	*Result, // [output]
	double	CutoffThreshold
);

///////////////////////////////////////////////////////////////////////////////
// 음절 단위 분석
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
	MAtRESULTS	*RestoredEJ, // 음운 복원된 어절
	MAtRESULTS	*Result, // [output]
	double	CutoffThreshold,
	int		BeamSize
);

#endif
