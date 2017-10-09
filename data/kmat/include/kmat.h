#ifndef KMAT_H
#define KMAT_H

#ifdef __cplusplus
extern "C" {
#endif

#define KMATcMAX_MA_RESULT_NUM	1024	// 한 어절의 최대 형태소 분석 결과의 수
#define KMATcMAX_EJ_NUM			1024	// 한 문장의 최대 어절 수

// 분석 단위 결정을 위한
// E, M, S, EM, ES, MS, EMS (7가지 조합)
#define EOJEOL_ANALYSIS		1
#define MORPHEME_ANALYSIS	2
#define SYLLABLE_ANALYSIS	4

typedef struct
{
	char		*str;	// 형태소 분석 결과
	double	prob;	// 확률값
} MAtRESULT;

typedef struct
{
	int		count;	// 형태소 분석 결과의 수
	MAtRESULT	mresult[KMATcMAX_MA_RESULT_NUM];
} MAtRESULTS;


////////////////////////////////////////////////////////////////
// 형태소 분석
////////////////////////////////////////////////////////////////
extern void *ma_Open
(
	char	*RscPath,	// [input] 리소스 경로
	int	PsUnit	// [input] 처리 단위 (어절,형태소,음절)
);

extern void ma_Close
(
	void	*Rsc,		// [input] 리소스
	int	PsUnit	// [input] 처리 단위 (어절,형태소,음절)
);

extern int ma_Exec
(
	const	char		*Eojeol,	// [input] 어절
	void			*Rsc,		// [input] 리소스
	int			PsUnit,	// [input] 처리 단위 (어절,형태소,음절)
	MAtRESULTS		*Result	// [output] 형태소 분석 결과
);

extern int ma_PrintResult
(
	FILE			*Fp,		// [output] 파일 포인터
	const char		*Eojeol,	// [input] 어절
	MAtRESULTS		*Result	// [input] 형태소 분석 결과
);

// 형태소 분석 결과를 위한 메모리 할당
extern MAtRESULTS *ma_New
(
	int	NumWord
);

// 형태소 분석 결과 메모리 해제
extern void ma_Free
(
	MAtRESULTS *Result
);

// 형태소 분석 결과(배열) 메모리 해제
extern void ma_FreeAll
(
	MAtRESULTS	*Result,
	int		NumWord
);

////////////////////////////////////////////////////////////////
// 형태소 분석 + 태깅
////////////////////////////////////////////////////////////////
extern void *kmat_Open
(
	char	*RSC_Path
);
	
extern void kmat_Close
(
	void	*Rsc
);

extern void kmat_Exec
(
	void		*Rsc,		// [input] 리소스
	char		**Words,	// [input] 형태소 분석 결과
	int		NumWord,	// [input] 어절의 수
	char		**Results	// [output] 태깅 결과
);

extern void kmat_PrintResult
(
	FILE		*Fp,		// [output]
	char		**Words,	// [input] 문장내 어절
	int		NumWord,	// [input] 어절의 수
	char		**Results	// [input] 태깅 결과
);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
