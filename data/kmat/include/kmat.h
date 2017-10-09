#ifndef KMAT_H
#define KMAT_H

#ifdef __cplusplus
extern "C" {
#endif

#define KMATcMAX_MA_RESULT_NUM	1024	// �� ������ �ִ� ���¼� �м� ����� ��
#define KMATcMAX_EJ_NUM			1024	// �� ������ �ִ� ���� ��

// �м� ���� ������ ����
// E, M, S, EM, ES, MS, EMS (7���� ����)
#define EOJEOL_ANALYSIS		1
#define MORPHEME_ANALYSIS	2
#define SYLLABLE_ANALYSIS	4

typedef struct
{
	char		*str;	// ���¼� �м� ���
	double	prob;	// Ȯ����
} MAtRESULT;

typedef struct
{
	int		count;	// ���¼� �м� ����� ��
	MAtRESULT	mresult[KMATcMAX_MA_RESULT_NUM];
} MAtRESULTS;


////////////////////////////////////////////////////////////////
// ���¼� �м�
////////////////////////////////////////////////////////////////
extern void *ma_Open
(
	char	*RscPath,	// [input] ���ҽ� ���
	int	PsUnit	// [input] ó�� ���� (����,���¼�,����)
);

extern void ma_Close
(
	void	*Rsc,		// [input] ���ҽ�
	int	PsUnit	// [input] ó�� ���� (����,���¼�,����)
);

extern int ma_Exec
(
	const	char		*Eojeol,	// [input] ����
	void			*Rsc,		// [input] ���ҽ�
	int			PsUnit,	// [input] ó�� ���� (����,���¼�,����)
	MAtRESULTS		*Result	// [output] ���¼� �м� ���
);

extern int ma_PrintResult
(
	FILE			*Fp,		// [output] ���� ������
	const char		*Eojeol,	// [input] ����
	MAtRESULTS		*Result	// [input] ���¼� �м� ���
);

// ���¼� �м� ����� ���� �޸� �Ҵ�
extern MAtRESULTS *ma_New
(
	int	NumWord
);

// ���¼� �м� ��� �޸� ����
extern void ma_Free
(
	MAtRESULTS *Result
);

// ���¼� �м� ���(�迭) �޸� ����
extern void ma_FreeAll
(
	MAtRESULTS	*Result,
	int		NumWord
);

////////////////////////////////////////////////////////////////
// ���¼� �м� + �±�
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
	void		*Rsc,		// [input] ���ҽ�
	char		**Words,	// [input] ���¼� �м� ���
	int		NumWord,	// [input] ������ ��
	char		**Results	// [output] �±� ���
);

extern void kmat_PrintResult
(
	FILE		*Fp,		// [output]
	char		**Words,	// [input] ���峻 ����
	int		NumWord,	// [input] ������ ��
	char		**Results	// [input] �±� ���
);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
