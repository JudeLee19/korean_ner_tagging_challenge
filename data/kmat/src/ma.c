//#define DEBUG

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h> // LONG_MAX

#include "kmat.h"
#include "ma.h"
#include "dafst.h"
#include "probtool.h"
#include "kmat_def.h"
#include "hsplit.h"
#include "phonetic_recover.h"
#include "get_morph_tag.h"
#include "binio.h"
#include "split_word.h"

/*
extern int num_eojeol_anal;
extern int num_morpheme_anal;
extern int num_syllable_anal;
extern int num_eojeol_anal_try;
extern int num_morpheme_anal_try;
extern int num_syllable_anal_try;
*/
#define RMEJ_FST			0
#define RMEJ_INFO			1
#define RMEJ_FREQ			2
#define PHONETIC_FST		3
#define PHONETIC_INFO		4
#define PHONETIC_FREQ		5
#define SYLLABLE_TAG_FST	6
#define SYLLABLE_TAG_INFO	7
//#define PHONETIC_INFO_PROB	8
#define SYLLABLE_DIC_FST	9
#define SYLLABLE_DIC_INFO	10
#define SYLLABLE_DIC_PROB	11
#define LEXICAL_FST		12
#define LEXICAL_INFO		13
#define LEXICAL_PROB		14
#define TRANSITION_FST		15
#define TRANSITION_PROB		16
#define M_MORPH_FST		17
#define M_MORPH_PROB		18
#define TAG_S_FST			19
#define TAG_S_PROB		20
#define SYLLABLE_S_FST		21
#define SYLLABLE_S_PROB		22
#define S_TRANSITION_FST	23

// rsc ȭ�ϸ�
// env.h�� ���ǵ� ����� �����
const char *ma_RSC_FILES[] = {
	"RMEJ.fst",			// 0
	"RMEJ.info",		// 1
	"RMEJ.freq",		// 2
	"PHONETIC.fst",		// 3
	"PHONETIC.info",		// 4
	"PHONETIC.freq",		// 5
	"SYLLABLE_TAG.fst",	// 6
	"SYLLABLE_TAG.info",	// 7
	"-------",	// 8
	"SYLLABLE_DIC.fst",	// 9
	"SYLLABLE_DIC.info",	// 10
	"SYLLABLE_DIC.prob",	// 11
	"LEXICAL.fst",		// 12
	"LEXICAL.info",		// 13
	"LEXICAL.prob",		// 14
	"TRANSITION.fst",		// 15
	"TRANSITION.prob",	// 16
	"M_MORPH.fst",		// 17
	"M_MORPH.prob",		// 18
	"TAG_S.fst",		// 19
	"TAG_S.prob",		// 20
	"SYLLABLE_S.fst",		// 21
	"SYLLABLE_S.prob",	// 22
	"S_TRANSITION.fst",	// 23
	NULL,
};

// ���� ����																									
// ȭ�ϸ��� ������ ���
char ma_RSC_FILES_PATH[25][100];

///////////////////////////////////////////////////////////////////////////////
int ma_CompareProb
(
	const void *a,
	const void *b
)
{
	MAtRESULT *x = (MAtRESULT *) a;
	MAtRESULT *y = (MAtRESULT *) b;

	if (x->prob > y->prob) return -1;
	else if (x->prob < y->prob) return 1;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
void *ma_Open
(
	char	*RscPath,	// [input] ���ҽ� ���
	int	PsUnit	// [input] ó�� ���� (����,���¼�,����)
)
{
	MAtRSC *rsc;
	int i;
	char rsc_path[300];
	
	strcpy( rsc_path, RscPath);
	
	// ���ҽ� ��� ���� (��� + ���ϸ�)
	for (i = 0; ma_RSC_FILES[i]; i++)
	{
		if (rsc_path[0])
		{
			int len = (int) strlen( rsc_path);
			
#ifdef WIN32 // ��������
			if (rsc_path[len-1] != '\\')
			{
				rsc_path[len] = '\\';
				rsc_path[len+1] = 0;
			}
#else // ���н��� ������
			if (rsc_path[len-1] != '/')
			{
				rsc_path[len] = '/';
				rsc_path[len+1] = 0;
			}
#endif
			sprintf( ma_RSC_FILES_PATH[i], "%s%s", rsc_path, ma_RSC_FILES[i]);
		}
		else
		{
			sprintf( ma_RSC_FILES_PATH[i], "%s", ma_RSC_FILES[i]);
		}
	}
	
	rsc = (MAtRSC *)malloc( sizeof( MAtRSC));
	assert( rsc != NULL);
	
	// �ʱ�ȭ
	rsc->e_rsc = NULL;
	rsc->phonetic = NULL;
	rsc->m_rsc = NULL;
	rsc->s_rsc = NULL;
	
	// ���� ���� �м��� �ϴ� ��츸
	if (PsUnit & EOJEOL_ANALYSIS)
	{
		rsc->e_rsc = ma_OpenE( ma_RSC_FILES_PATH[RMEJ_FST], 
					ma_RSC_FILES_PATH[RMEJ_INFO], 
					ma_RSC_FILES_PATH[RMEJ_FREQ]);

		if (rsc->e_rsc == NULL) return NULL;
	}

	// ���¼ҳ� ���� ���� �м��� �ؾ�
	// ���� ���� ����
	if (PsUnit & (MORPHEME_ANALYSIS | SYLLABLE_ANALYSIS))
	{
		{
			rsc->phonetic = phonetic_Open( ma_RSC_FILES_PATH[PHONETIC_FST], 
								ma_RSC_FILES_PATH[PHONETIC_INFO],
								ma_RSC_FILES_PATH[PHONETIC_FREQ]);

			if (rsc->phonetic == NULL) return NULL;
		}
	}
	
	// ���¼� ���� �м��� �ϴ� ��츸
	if (PsUnit & MORPHEME_ANALYSIS)
	{
		rsc->m_rsc = ma_OpenM( ma_RSC_FILES_PATH[LEXICAL_FST], 
					ma_RSC_FILES_PATH[LEXICAL_INFO],
					ma_RSC_FILES_PATH[LEXICAL_PROB],
					ma_RSC_FILES_PATH[TRANSITION_FST],
					ma_RSC_FILES_PATH[TRANSITION_PROB],
					ma_RSC_FILES_PATH[M_MORPH_FST],
					ma_RSC_FILES_PATH[M_MORPH_PROB]);
		
		if (rsc->m_rsc == NULL) return NULL;
	}

	// ���� ���� �м��� �ϴ� ��츸
	if (PsUnit & SYLLABLE_ANALYSIS)
	{
		rsc->s_rsc = ma_OpenS( ma_RSC_FILES_PATH[TAG_S_FST], 
					ma_RSC_FILES_PATH[TAG_S_PROB],
					ma_RSC_FILES_PATH[SYLLABLE_S_FST], 
					ma_RSC_FILES_PATH[SYLLABLE_S_PROB],
					ma_RSC_FILES_PATH[S_TRANSITION_FST],
					ma_RSC_FILES_PATH[SYLLABLE_TAG_FST],
					ma_RSC_FILES_PATH[SYLLABLE_TAG_INFO]);

		if (rsc->s_rsc == NULL) return NULL;
	}

	return (MAtRSC *)rsc;
}

///////////////////////////////////////////////////////////////////////////////
void ma_Close
(
	void	*Rsc,
	int	 PsUnit	// [input] ó�� ���� (����,���¼�,����)
)
{
	MAtRSC *rsc = (MAtRSC *) Rsc;

	assert( rsc != NULL);
	
	if (PsUnit & EOJEOL_ANALYSIS) // ���� ���� �м��� �ϴ� ���
	{
		ma_CloseE( rsc->e_rsc);
	}

	if (PsUnit & (MORPHEME_ANALYSIS | SYLLABLE_ANALYSIS)) // ���¼ҳ� ���� ���� �м��� �ϴ� ���
	{
		phonetic_Close( rsc->phonetic);
	}

	// ���¼� ���� �м��� �ϴ� ���
	if (PsUnit & MORPHEME_ANALYSIS)
	{
		ma_CloseM( rsc->m_rsc);
	}

	// ���� ���� �м��� �ϴ� ���
	if (PsUnit & SYLLABLE_ANALYSIS)
	{
		ma_CloseS( rsc->s_rsc);
	}

	free( rsc);
	rsc = NULL;
}

///////////////////////////////////////////////////////////////////////////////
MAtRESULTS *ma_New
(
	int	NumWord
)
{
	MAtRESULTS *result = (MAtRESULTS *)malloc( sizeof( MAtRESULTS) * NumWord);
	
	assert( result != NULL);
	
	//result = memset( result, 0, NumWord * sizeof( MAtRESULTS));
	// �ʱ�ȭ
	int i;
	for (i = 0; i < NumWord; i++) result[i].count = 0;
	
	return result;
}

///////////////////////////////////////////////////////////////////////////////
// ���¼� �м� ����� ���� ����ü(�޸�) ����
void ma_Free
(
	MAtRESULTS *Result
)
{
	int j = 0;

	if (Result == NULL) return;

	for (j = 0; j < Result->count; j++)
	{
		Result->mresult[j].prob = 0.0; // log(1)
		
		if (Result->mresult[j].str)
		{
			//**/fprintf( stderr, "\t[%s] has been freed!\n", Result->mresult[j].str);
			free( Result->mresult[j].str);
			Result->mresult[j].str = NULL;
		}
	}
	Result->count = 0;
	
	free( Result);
	Result = NULL;
}

///////////////////////////////////////////////////////////////////////////////
void ma_FreeAll
(
	MAtRESULTS	*Result,
	int		NumWord
)
{
	int i, j;
	
	if (Result == NULL) return;

	for (i = 0; i < NumWord; i++)
	{
		assert( Result[i].count < KMATcMAX_MA_RESULT_NUM);
		
		for (j = 0; j < Result[i].count; j++)
		{
			Result[i].mresult[j].prob = 0.0; // log(1)
		
			if (Result[i].mresult[j].str)
			{
				free( Result[i].mresult[j].str);
				Result[i].mresult[j].str = NULL;
			}
		}
		Result[i].count = 0;
	}

	free( Result);
	Result = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// ��� ���
// ���ϰ� : �м� ����� ��
int ma_PrintResult
(
	FILE			*Fp,		// [output] ���� ������
	const char		*Eojeol,	// [input] ����
	MAtRESULTS		*Result	// [input] ���¼� �м� ���
)
{
	int num_result = Result->count;
	int i = 0;
	
	if (num_result == 0) return 0;

	// ������ ���
	fprintf( Fp, "%s", Eojeol);
	
	// �м� ��� ���
	for (i = 0; i < num_result; i++) 
	{
		// �м� ���, Ȯ��
		fprintf( Fp, "\t%s\t%lf\n", Result->mresult[i].str, Result->mresult[i].prob); // 12.11e
	}

	return num_result; // �м��� ����� ��
}

///////////////////////////////////////////////////////////////////////////////
static void __AssignResult
(
	MAtRESULTS	*Result,
	const char	*Eojeol,
	const char	*Tag
)
{
	char *not_analyzed;
	
	not_analyzed = (char *)malloc( strlen( Eojeol) + strlen( Tag) + 2);

	sprintf( not_analyzed, "%s/%s", Eojeol, Tag); // �Է� ����, �±�

	Result->mresult[Result->count].str = strdup( not_analyzed);
	Result->mresult[Result->count].prob = 0.0; // Ȯ���� 0 = log(1)�� �ο���
	Result->count++;
	
	free( not_analyzed);
}

///////////////////////////////////////////////////////////////////////////////
// Ȯ���� ���¼� �м�
// ���ϰ� : 0 = �м���� ����, 1 = �м� ��� ����
// Eojeol : �Է� ����
// CutoffThreshold : (Ȯ������ ����) �м� ������� ���̱� ���� �Ӱ谪
// analyzed_result : �м� ��� (Ȯ������ �α�)
static int morphological_analysis
(
	void		*Rsc,
	const char	*Eojeol,
	int		IsFirst,	// ���� ���ۺ��ΰ�?
	int		IsLast,	// ���� ������ΰ�?
	double	cutoff_threshold_m,
	double	cutoff_threshold_s,
	int		BeamSize,
	int		PsUnit,			// [input] ó�� ���� (����,���¼�,����)
	MAtRESULTS	*Result
)
{
	int num_result = 0; // �м��� ����� ��
	MAtRSC *rsc = (MAtRSC *) Rsc;

	//**/fprintf( stderr, "IsFirst = %d, IsLast = %d\n", IsFirst, IsLast);

	// ���� ���� �м�
	// ������ ������ ��츸 ���� ���� �м� �õ�
	if (IsFirst && PsUnit & EOJEOL_ANALYSIS)
	{
		num_result = ma_ExecE( rsc->e_rsc, Eojeol, Result);
	 
		// ���� ���� �м� ����� ������
		if (num_result) 
		{
#ifdef DEBUG
			fprintf( stderr, "���� ���� �м� ��� ����\n");
#endif
			return EOJEOL;
		}
#ifdef DEBUG
		else
	 	{
			fprintf( stderr, "���� ���� �м� ��� ����\n");
		}
#endif
	}

	// ���¼� ���� �м��̳� ���� ���� �м��� ������ ���� ���� ����
	if (PsUnit & (MORPHEME_ANALYSIS | SYLLABLE_ANALYSIS)) 
	{
		MAtRESULTS *restored_ej = NULL; // ���� ���� ���� (Ȯ�� + ������ ����)

		// �޸� �Ҵ�
		restored_ej = (MAtRESULTS *)malloc( sizeof( MAtRESULTS));
		assert( restored_ej != NULL);
		
		// �ʱ�ȭ
		restored_ej->count = 0;
		
		// ���� ���� ����
			
#ifdef DEBUG
		fprintf( stderr, "���� ���� ����\n");
#endif
		// �Է� : Eojeol
		// ��� : restored_ej
		// ���ϰ� : ������ ������ ��
		if ( !phonetic_Recover( rsc->phonetic, Eojeol, restored_ej))
		{
#ifdef DEBUG
			fprintf( stderr, "������ ���� ���� [%s]\n", Eojeol);
#endif			
			// �޸� ����
			ma_Free( restored_ej);
			return 0;
		}

#ifdef DEBUG
		fprintf( stderr, "������ ������ �� = %d\n", restored_ej->count);

		// ������ ������ ���
		int i;
		for (i = 0; i < restored_ej->count; i++)
		{
			fprintf( stderr, "������ ���� (in %s): %s\t%lf\n", __FILE__, restored_ej->mresult[i].str, restored_ej->mresult[i].prob);
		}
#endif

		// ���¼� ���� �м�
		if (PsUnit & MORPHEME_ANALYSIS) 
		{
//			num_morpheme_anal_try++;

			num_result = ma_ExecM( rsc->m_rsc, restored_ej, IsFirst, IsLast, Result, cutoff_threshold_m);

			// ���¼� ���� �м� ����� ������
			if (num_result) 
			{
				// �޸� ����
				ma_Free( restored_ej);
#ifdef DEBUG
				fprintf( stderr, "���¼� ���� �м� ��� ����\n");
#endif
//				num_morpheme_anal++;
				return MORPHEME;
			}
#ifdef DEBUG
			else
	 		{
				fprintf( stderr, "���¼� ���� �м� ��� ����\n");
			}
#endif
		}

		// ���� ���� �м�
		if (PsUnit & SYLLABLE_ANALYSIS) 
		{
//			num_syllable_anal_try++;

			num_result = ma_ExecS( rsc->s_rsc, restored_ej, Result,
							cutoff_threshold_s, BeamSize);

			// ���� ���� �м� ����� ������
			if (num_result) 
			{
#ifdef DEBUG
			fprintf( stderr, "���� ���� �м� ��� ����\n");
#endif

				// �޸� ����
				ma_Free( restored_ej);

//				num_syllable_anal++;
				return SYLLABLE;
			}
#ifdef DEBUG
			else
		 	{
				fprintf( stderr, "���� ���� �м� ��� ����\n");
			}
#endif
		}

		// �޸� ����
		ma_Free( restored_ej);
	}

	// ������� ���� ���� �м� ����� ���� ��
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
const char DELIM_CHAR[][3] = 
{
	"(",
	",",
	"-",
	".",
	"/",
	":",
	";",
	"~",
	"��",
	"��",
	"��",
	"��",
	"��",
	"��",
	"��",
	"��",
	"��",
	"��",
	"��",
	"��",
};

// �� ������ �����ϴ� �Լ�
// �ϼ��� �ڵ忡�� �����
// return value: �κ� ������ ��
/*
int SplitWord
(
	const char	*Eojeol,	// [input]
	int		Len,		// [input]
	char		*SubEjs[]	// [output]
)
{
	char chars[Len][3];
	int num_chars;
	
	int char_freq[20] = {0,};
	int i;
	int max = 0;
	int max_index = -1;
	unsigned char first, second;
	
	num_chars = split_by_char( (char *)Eojeol, chars);
//	fprintf( stderr, "num_char = %d\n", num_chars);
	
	// �и� ������ �󵵸� ����.
	for (i = 0; i < num_chars; i++)
	{
#ifdef DEBUG
		fprintf( stderr, "char = /%s/\n", chars[i]);
#endif
		first = (unsigned char)chars[i][0];
		second = (unsigned char)chars[i][1];
		
		switch (first)
		{
			case FIL:
				switch (second)
				{
					case 0x28: // (
						char_freq[0]++;
						break;
					case 0x2C: // ,
						char_freq[1]++;
						break;
					case 0x2D: // -
						char_freq[2]++;
						break;
					case 0x2E: // .
						char_freq[3]++;
						break;
					case 0x2F: // /
						char_freq[4]++;
						break;
					case 0x3A: // :
						char_freq[5]++;
						break;
					case 0x3B: // ;
						char_freq[6]++;
						break;
					case 0x7E: // ~
						char_freq[7]++;
						break;
				}
				break;
			case 0xA1:
				switch (second)
				{
					case 0xA4: // ��
						char_freq[8]++;
						break;
					case 0xAD: // �� (����)
						char_freq[9]++;
						break;
					case 0xDE: // ��
						char_freq[10]++;
						break;
					case 0xE2: // ��
						char_freq[11]++;
						break;
					case 0xE3: // ��
						char_freq[12]++;
						break;
					case 0xE6: // ��
						char_freq[13]++;
						break;
				}
				break;
			case 0xA2:
				if (second == 0xB9) // ��
					char_freq[14]++;
				else if (second == 0xBA) // ��
					char_freq[15]++;
				break;
			case 0xA3:
				if (second == 0xAE) // ��
					char_freq[16]++;
				else if (second == 0xDE) // ��
					char_freq[17]++;
				break;
			case 0xA4:
				if (second == 0xFD) // ��
					char_freq[18]++;
				break;
			case 0xA5:
				if (second == 0xB0) // ��
					char_freq[19]++;
				break;
		} // end of switch
	} // end of for
	
	for (i = 0; i < 20; i++)
	{
		if (char_freq[i] >= max) // ��ȣ ��� ���� : ������ ��� �޹��ڸ� ��ȣ
		{
			max = char_freq[i];
			max_index = i;
		}
	}
	if (max == 0)
	{
		fprintf( stderr, "Too long word! It can't be splitted! (length = %d)\t%s\n", Len, Eojeol);
		return 0;
	}
	
	const char *delim = DELIM_CHAR[max_index];
	
#ifdef DEBUG
	fprintf( stderr, "delim = %s\n%s\n", delim, Eojeol);
#endif
	int num_subej = 0;
	int delim_len = strlen( delim);
	char *from = (char *)Eojeol;
	char *to = strstr( from, delim);
	int fromto;
	
	// ���� ��ġ���� �����ڰ� �߰ߵǸ� ���� ���� ��ġ���ͷ� �����ϰ� ��
	if (to == from)
	{
		to = strstr( from+delim_len, delim);
	}
	
	while (to != NULL)
	{
		fromto = to-from;
		SubEjs[num_subej] = (char *)malloc( sizeof( char) * (fromto+1));
		strncpy( SubEjs[num_subej], &Eojeol[from-Eojeol], fromto);
		SubEjs[num_subej][fromto] = 0;
		num_subej++;
		
		from = to;
		if (*(from+delim_len) == 0) break; // ���� �������� �ٴٸ��� ���⼭ ����
		to = strstr( from+delim_len, delim);
	}
	SubEjs[num_subej++] = strdup( from);
	
	return num_subej;
}
*/

///////////////////////////////////////////////////////////////////////////////
void GetFirstTag
(
	char	*FirstTag,	// [output] ù �±�
	char	*MA_Result		// [input] ���¼� �м� ���
)
{
	char *ptr1 = strchr( MA_Result, '/');
	if (*(++ptr1) == '/') ptr1++;
	char *ptr2 = strchr( ptr1, '+');
	
	if (ptr2 == NULL) strcpy( FirstTag, ptr1);
	else
	{
		strncpy( FirstTag, ptr1, ptr2-ptr1);
		FirstTag[ptr2-ptr1] = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
// �� ���¼� �м� ����� ��ģ��.
// Result <- Result + AddOn
int ma_IntegrateResult
(
	MAtRSC_M	*Rsc,
	MAtRESULTS	*Result,	// [in/output]
	MAtRESULTS	*AddOn,	// [input]
	int		IsLast,	// [input] ������ �κ� �����ΰ�?
	double	CutoffThreshold,
	int		TightCheck	// ���� �˻縦 ö���ϰ� �� ���ΰ�?
)
{
	int i;
	int j;
	
	MAtRESULTS	*new_results;
	double max_prob;
	double cur_prob;
	PROBtFST *rsc_trans = Rsc->transition;	// ���� Ȯ��
	char cur_first_pos[8];
	double trans_prob;
	
	////////////////////////////////////////////////////////////////////////////////
	// Result�� �� ���	
	if (Result->count == 0)
	{
		for (j = 0; j < AddOn->count; j++)
		{
			// ù��° ǰ�� �˾Ƴ���
			GetFirstTag( cur_first_pos, AddOn->mresult[j].str);
			
			// ���� ó�� �±׿��� ���Ἲ �˻�
			// ���� �Ұ����ϸ�
			// P(ùǰ�� | t0)
			trans_prob = prob_GetFSTProb2( rsc_trans, cur_first_pos, (char *)BOW_TAG_1);
			
			if (TightCheck && (trans_prob <= PROBcLOG_ALMOST_ZERO))
			{
#ifdef DEBUG
				fprintf(stderr, "�Ұ� %s -> %s\n", BOW_TAG_1, cur_first_pos);
#endif
				continue;
			}

			Result->mresult[Result->count].str = strdup( AddOn->mresult[j].str);
			Result->mresult[Result->count].prob = AddOn->mresult[j].prob + trans_prob;
			Result->count++;
		}
		
		if (Result->count == 0) return 0; // ��� ����� ���Ἲ �˻翡�� �����ϴ� ���
		
		// Ȯ�������� ����
		qsort( Result->mresult, Result->count, sizeof( MAtRESULT), ma_CompareProb);
		
		return 1;
	}
	
	////////////////////////////////////////////////////////////////////////////////
	char *prev_ma_str;
	char *cur_ma_str;
	char *prev_last_pos;
	double trans_prob2 = 0.0; // �ʱ�ȭ
	int loop_quit = 0;
	
	new_results = (MAtRESULTS *)malloc( sizeof( MAtRESULTS)); //ma_New( 1);
	new_results->count = 0;
	
	max_prob = -LONG_MAX;
	
	for (i = 0; i < Result->count; i++)
	{
		/**/assert( Result->mresult[i].str != NULL);
		prev_ma_str = Result->mresult[i].str;
		
		// ������ ǰ�� �˾Ƴ���
		prev_last_pos = strrchr( prev_ma_str, '/')+1;
		//**/fprintf( stderr, "last pos = %s (%s)\n", prev_last_pos, prev_ma_str);
		
		for (j = 0; j < AddOn->count; j++)
		{
			/**/assert( AddOn->mresult[j].str != NULL);
			cur_ma_str = AddOn->mresult[j].str;
			
			// ù��° ǰ�� �˾Ƴ���
			GetFirstTag( cur_first_pos, cur_ma_str);

			//**/fprintf( stderr, "first pos = %s (%s)\n", cur_first_pos, cur_ma_str);
			
			// ����Ȯ��
			trans_prob = prob_GetFSTProb2( rsc_trans, cur_first_pos, prev_last_pos);
			if (TightCheck && (trans_prob <= PROBcLOG_ALMOST_ZERO)) // ���� �Ұ����ϸ� // Ȯ���� �� ��!
			{
#ifdef DEBUG
				fprintf( stderr, "�Ұ� %s -> %s\n", prev_last_pos, cur_first_pos);
#endif
				continue;
			}
			
			// ������ �κ� �����̸� ������ �±׿��� ���� ���ɼ� ���� �˻� �� ���� Ȯ�� ���
			if (IsLast)
			{
				// ������ ǰ�� �˾Ƴ���
				char *cur_last_pos = strrchr( cur_ma_str, '/')+1;
				//**/fprintf( stderr, "last pos = %s (%s)\n", cur_last_pos, cur_ma_str);
				
				// ����Ȯ�� // P( EOW_TAG | �������±�)
				trans_prob2 = prob_GetFSTProb2( rsc_trans, (char *)EOW_TAG, cur_last_pos);
				if (TightCheck && (trans_prob2 <= PROBcLOG_ALMOST_ZERO)) // ���� �Ұ����ϸ� // Ȯ���� �� ��!
				{
#ifdef DEBUG
					fprintf( stderr, "�Ұ� %s -> %s\n", cur_last_pos, (char *)EOW_TAG);
#endif
					continue;
				}
			}
			
			// Ȯ����
			cur_prob = Result->mresult[i].prob + AddOn->mresult[j].prob + trans_prob + trans_prob2;
			if (max_prob < cur_prob) max_prob = cur_prob;
			
			// �ְ� Ȯ���� ���� ������� �αװ��� ���� ����ġ �̻��̸� ����
			if (max_prob - cur_prob > CutoffThreshold)
			{
#ifdef DEBUG
				fprintf( stderr, "cutoffed! in %s (%d) line\n", __FILE__, __LINE__);
#endif
				continue; // break�� ������ (�ڿ� ������ ���� �� ���� Ȯ���� ���� ���� �����Ƿ�)
			}

			new_results->mresult[new_results->count].prob = cur_prob;
			
			// �޸� �Ҵ�
			char *str = (char *)malloc( strlen( prev_ma_str) + strlen( cur_ma_str) + 2);
			
			// ���ڿ� ��ħ
			sprintf( str, "%s+%s", prev_ma_str, cur_ma_str);
			new_results->mresult[new_results->count].str = str;

			new_results->count++;
			
			if (new_results->count >= KMATcMAX_MA_RESULT_NUM-1)
			{
#ifdef DEBUG
				fprintf( stderr, "more than KMATcMAX_MA_RESULT_NUM(%d) results\n", KMATcMAX_MA_RESULT_NUM);
#endif
				loop_quit = 1;
				break;
			}
		} // end of for
		
		if (loop_quit)
		{
			break;
		}
	} // end of for
	
	// ���� �м� ��� ����
	for (i = 0; i < Result->count; i++)
	{
		if ( Result->mresult[i].str)
		{
			free( Result->mresult[i].str);
			//Result->mresult[i].str = NULL;
		}
	}
	Result->count = 0;

	if (new_results->count == 0)
	{
		ma_Free( new_results);
		return 0; // ��� ����� ���Ἲ �˻翡�� �����ϴ� ���
	}

	// ����
	qsort( new_results->mresult, new_results->count, sizeof( MAtRESULT), ma_CompareProb);
	
	max_prob = new_results->mresult[0].prob;

	// ���� �м� ��� <- ���ο� �м� ��� ����
	for (j = 0; j < new_results->count; j++)
	{
		cur_prob = new_results->mresult[j].prob;

		if (max_prob - cur_prob > CutoffThreshold)
		{
#ifdef DEBUG
			fprintf( stderr, "cutoffed! in %s (%d) line\n", __FILE__, __LINE__);
#endif
			break;
		}
		
		Result->mresult[Result->count].str = strdup( new_results->mresult[j].str);
		Result->mresult[Result->count].prob = cur_prob;
		Result->count++;
	}
	
	// ���ο� �м� ��� ����
	ma_Free( new_results);
	
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
int ma_Exec            
(                      
	const char		*Eojeol,	// [input] ����
	void			*Rsc,		// [input] ���ҽ�
	int			PsUnit,	// [input] ó�� ���� (����,���¼�,����)
	MAtRESULTS		*Result	// [output] ���¼� �м� ���
)
{
	int ret;
	int tight_check = 1;
	
	// �ʱ�ȭ
	Result->count = 0;

#ifdef DEBUG
	fprintf( stderr, "---------------\n%s\t(line %d in %s)\n", Eojeol, __LINE__, __FILE__);
#endif

	// ���� ���� �м�
	if (PsUnit & EOJEOL_ANALYSIS) 
	{
		int num_result;
		
//		/**/num_eojeol_anal_try++;
		num_result = ma_ExecE( ((MAtRSC *)Rsc)->e_rsc, Eojeol, Result);
	 
		// ���� ���� �м� ����� ������
		if (num_result) 
		{
//			/**/num_eojeol_anal++;
#ifdef DEBUG
			fprintf( stderr, "���� ���� �м� ��� ����\n");
#endif
			return Result->count;
		}
#ifdef DEBUG
		else
	 	{
			fprintf( stderr, "���� ���� �м� ��� ����\n");
		}
#endif
	}
	
	int len = strlen( Eojeol);
	char *subEjs[len];
	int ctypes[len];
	int num_subejs;
	int i;
	MAtRESULTS *ma_results = NULL;	// ���¼� �м� ��� ����
	
	// ������ ���������� ���� ����
	num_subejs = SplitWord( Eojeol, len, subEjs, ctypes);
	
#ifdef DEBUG
	fprintf( stderr, "# of sub-Eojeols = %d\n", num_subejs);
#endif
	
	if (num_subejs == 0) return 0;

	// �κ� ������ ���� ���� ������ ��°�� �м� �õ�	
	if (num_subejs < 10)
	{
		ret = morphological_analysis( Rsc, Eojeol, 1, 1, 10, 10, 15, PsUnit, Result);
		
		// �м� ����� ������ ����
		if (ret != 0)
		{
#ifdef DEBUG
			fprintf( stderr, "��ü ���� �м� ���\n");
			ma_PrintResult( stderr, Eojeol, Result);
#endif

			for (i = 0; i < num_subejs; i++) free( subEjs[i]);
			return Result->count;
		}
#ifdef DEBUG
			fprintf( stderr, "��ü ���� �м� ��� ����\n");
#endif
	}
	
	ma_results = ma_New( num_subejs);
	
	assert( ma_results != NULL);
	
	// �κ� ������ ���� �Ӱ谪 �̻��� ���� ǰ�� ���� ���ɼ��� ��� cutoff ��Ű�� ����
	//if (num_subejs >= 10) tight_check = 0;
	tight_check = 0;
	
	// �� �̻��� �κ� ������ �ִ� ���
	for (i = 0; i < num_subejs; i++)
	{
		char *subej = subEjs[i];
#ifdef DEBUG
		fprintf( stderr, "sub-ej[%d]: %s (%s)\n", i, subej, UHCcCTYPE[ctypes[i]]);
#endif

		switch (ctypes[i])
		{
			case UHC_HANGEUL: // �ѱ��� �󵵰� ���� �����Ƿ� ���� ���� ����
			case ASC_PUNCT:
			case UHC_SYMBOL:
				ret = morphological_analysis( Rsc, subej, i == 0 ? 1 : 0, i == (num_subejs-1) ? 1 : 0, 10, 10, 15, PsUnit, &ma_results[i]);
				
				if (ret == 0)
				{
					if (ctypes[i] == UHC_SYMBOL || ctypes[i] == ASC_PUNCT) // ��ȣ �̺м��� SW
						__AssignResult( &ma_results[i], subej, "SW");
					else __AssignResult( &ma_results[i], subej, "NA"); // �ѱ� �̺м��� NA
				}
				break;
			case ASC_DIGIT:
			case UHC_DIGIT:
				__AssignResult( &ma_results[i], subej, "SN");
				break;

			case ASC_ALPHA:
			case UHC_LATIN:
			case UHC_HIRAGANA:
			case UHC_KATAKANA:
				__AssignResult( &ma_results[i], subej, "SL");
				break;

			case UHC_HANJA:
				__AssignResult( &ma_results[i], subej, "SH");
				break;
			
			case ASC_CONTROL:
			case ASC_UNKNOWN:
			case UHC_UNKNOWN:
			case UHC_FILLER:
			case UHC_USERAREA:
				__AssignResult( &ma_results[i], subej, "NA");
				break;

			case UHC_GREEK:
			case UHC_CYRIL:
			case UHC_BOXDRAW:
				__AssignResult( &ma_results[i], subej, "SW");
				break;
			case UHC_JAEUM:
			case UHC_MOEUM:
				__AssignResult( &ma_results[i], subej, "NNG");
				break;
			case UHC_YESJAEUM:
			case UHC_YESMOEUM:
				// �Ʒ����̸� (������� ����� ����ؼ� ��������� ����ϴ� ��찡 ����)
				if (strlen( subej) == 2 && (unsigned char)subej[0] == 0xA4 && (unsigned char)subej[1] == 0xFD)
				{
					__AssignResult( &ma_results[i], subej, "SP");
				}
				else __AssignResult( &ma_results[i], subej, "SW");
				break;

			default: // ����� �� ���� ����
				assert( 0);
				__AssignResult( &ma_results[i], subej, "NA");
				break;
		} // end of switch
		
#ifdef DEBUG
		// �κ� �м� ��� ���
		fprintf( stderr, "�κ� ���� �м� ���\n");
		ma_PrintResult( stderr, subej, &ma_results[i]);
#endif
		// �м� ��� ����
		ret = ma_IntegrateResult( ((MAtRSC *)Rsc)->m_rsc, Result, &ma_results[i], i == (num_subejs-1) ? 1 : 0, 10, tight_check);
		
		if (ret == 0)
		{
			__AssignResult( Result, Eojeol, "NA");
			break;
		}
	} // end of for
	
	for (i = 0; i < num_subejs; i++) free( subEjs[i]);
	
	// �м� ����� ���� �޸� ����
	ma_FreeAll( ma_results, num_subejs);

	return Result->count;
}
