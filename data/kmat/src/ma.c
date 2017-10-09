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

// rsc 화일명
// env.h에 정의된 상수와 연결됨
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

// 전역 변수																									
// 화일명을 포함한 경로
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
	char	*RscPath,	// [input] 리소스 경로
	int	PsUnit	// [input] 처리 단위 (어절,형태소,음절)
)
{
	MAtRSC *rsc;
	int i;
	char rsc_path[300];
	
	strcpy( rsc_path, RscPath);
	
	// 리소스 경로 설정 (경로 + 파일명)
	for (i = 0; ma_RSC_FILES[i]; i++)
	{
		if (rsc_path[0])
		{
			int len = (int) strlen( rsc_path);
			
#ifdef WIN32 // 윈도우즈
			if (rsc_path[len-1] != '\\')
			{
				rsc_path[len] = '\\';
				rsc_path[len+1] = 0;
			}
#else // 유닉스나 리눅스
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
	
	// 초기화
	rsc->e_rsc = NULL;
	rsc->phonetic = NULL;
	rsc->m_rsc = NULL;
	rsc->s_rsc = NULL;
	
	// 어절 단위 분석을 하는 경우만
	if (PsUnit & EOJEOL_ANALYSIS)
	{
		rsc->e_rsc = ma_OpenE( ma_RSC_FILES_PATH[RMEJ_FST], 
					ma_RSC_FILES_PATH[RMEJ_INFO], 
					ma_RSC_FILES_PATH[RMEJ_FREQ]);

		if (rsc->e_rsc == NULL) return NULL;
	}

	// 형태소나 음절 단위 분석을 해야
	// 음운 복원 정보
	if (PsUnit & (MORPHEME_ANALYSIS | SYLLABLE_ANALYSIS))
	{
		{
			rsc->phonetic = phonetic_Open( ma_RSC_FILES_PATH[PHONETIC_FST], 
								ma_RSC_FILES_PATH[PHONETIC_INFO],
								ma_RSC_FILES_PATH[PHONETIC_FREQ]);

			if (rsc->phonetic == NULL) return NULL;
		}
	}
	
	// 형태소 단위 분석을 하는 경우만
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

	// 음절 단위 분석을 하는 경우만
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
	int	 PsUnit	// [input] 처리 단위 (어절,형태소,음절)
)
{
	MAtRSC *rsc = (MAtRSC *) Rsc;

	assert( rsc != NULL);
	
	if (PsUnit & EOJEOL_ANALYSIS) // 어절 단위 분석을 하는 경우
	{
		ma_CloseE( rsc->e_rsc);
	}

	if (PsUnit & (MORPHEME_ANALYSIS | SYLLABLE_ANALYSIS)) // 형태소나 음절 단위 분석을 하는 경우
	{
		phonetic_Close( rsc->phonetic);
	}

	// 형태소 단위 분석을 하는 경우
	if (PsUnit & MORPHEME_ANALYSIS)
	{
		ma_CloseM( rsc->m_rsc);
	}

	// 음절 단위 분석을 하는 경우
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
	// 초기화
	int i;
	for (i = 0; i < NumWord; i++) result[i].count = 0;
	
	return result;
}

///////////////////////////////////////////////////////////////////////////////
// 형태소 분석 결과를 위한 구조체(메모리) 해제
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
// 결과 출력
// 리턴값 : 분석 결과의 수
int ma_PrintResult
(
	FILE			*Fp,		// [output] 파일 포인터
	const char		*Eojeol,	// [input] 어절
	MAtRESULTS		*Result	// [input] 형태소 분석 결과
)
{
	int num_result = Result->count;
	int i = 0;
	
	if (num_result == 0) return 0;

	// 원어절 출력
	fprintf( Fp, "%s", Eojeol);
	
	// 분석 결과 출력
	for (i = 0; i < num_result; i++) 
	{
		// 분석 결과, 확률
		fprintf( Fp, "\t%s\t%lf\n", Result->mresult[i].str, Result->mresult[i].prob); // 12.11e
	}

	return num_result; // 분석된 결과의 수
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

	sprintf( not_analyzed, "%s/%s", Eojeol, Tag); // 입력 어절, 태그

	Result->mresult[Result->count].str = strdup( not_analyzed);
	Result->mresult[Result->count].prob = 0.0; // 확률은 0 = log(1)로 부여함
	Result->count++;
	
	free( not_analyzed);
}

///////////////////////////////////////////////////////////////////////////////
// 확률적 형태소 분석
// 리턴값 : 0 = 분석결과 없음, 1 = 분석 결과 있음
// Eojeol : 입력 어절
// CutoffThreshold : (확률값이 낮은) 분석 결과수를 줄이기 위한 임계값
// analyzed_result : 분석 결과 (확률값은 로그)
static int morphological_analysis
(
	void		*Rsc,
	const char	*Eojeol,
	int		IsFirst,	// 어절 시작부인가?
	int		IsLast,	// 어절 종료부인가?
	double	cutoff_threshold_m,
	double	cutoff_threshold_s,
	int		BeamSize,
	int		PsUnit,			// [input] 처리 단위 (어절,형태소,음절)
	MAtRESULTS	*Result
)
{
	int num_result = 0; // 분석된 결과의 수
	MAtRSC *rsc = (MAtRSC *) Rsc;

	//**/fprintf( stderr, "IsFirst = %d, IsLast = %d\n", IsFirst, IsLast);

	// 어절 단위 분석
	// 어절의 시작인 경우만 어절 단위 분석 시도
	if (IsFirst && PsUnit & EOJEOL_ANALYSIS)
	{
		num_result = ma_ExecE( rsc->e_rsc, Eojeol, Result);
	 
		// 어절 단위 분석 결과가 있으면
		if (num_result) 
		{
#ifdef DEBUG
			fprintf( stderr, "어절 단위 분석 결과 있음\n");
#endif
			return EOJEOL;
		}
#ifdef DEBUG
		else
	 	{
			fprintf( stderr, "어절 단위 분석 결과 없음\n");
		}
#endif
	}

	// 형태소 단위 분석이나 음절 단위 분석이 있으면 음운 현상 복원
	if (PsUnit & (MORPHEME_ANALYSIS | SYLLABLE_ANALYSIS)) 
	{
		MAtRESULTS *restored_ej = NULL; // 음운 현상 복원 (확률 + 복원된 어절)

		// 메모리 할당
		restored_ej = (MAtRESULTS *)malloc( sizeof( MAtRESULTS));
		assert( restored_ej != NULL);
		
		// 초기화
		restored_ej->count = 0;
		
		// 음운 현상 복원
			
#ifdef DEBUG
		fprintf( stderr, "음운 복원 시작\n");
#endif
		// 입력 : Eojeol
		// 출력 : restored_ej
		// 리턴값 : 복원된 어절의 수
		if ( !phonetic_Recover( rsc->phonetic, Eojeol, restored_ej))
		{
#ifdef DEBUG
			fprintf( stderr, "복원된 어절 없음 [%s]\n", Eojeol);
#endif			
			// 메모리 해제
			ma_Free( restored_ej);
			return 0;
		}

#ifdef DEBUG
		fprintf( stderr, "복원된 어절의 수 = %d\n", restored_ej->count);

		// 복원된 어절들 출력
		int i;
		for (i = 0; i < restored_ej->count; i++)
		{
			fprintf( stderr, "복원된 어절 (in %s): %s\t%lf\n", __FILE__, restored_ej->mresult[i].str, restored_ej->mresult[i].prob);
		}
#endif

		// 형태소 단위 분석
		if (PsUnit & MORPHEME_ANALYSIS) 
		{
//			num_morpheme_anal_try++;

			num_result = ma_ExecM( rsc->m_rsc, restored_ej, IsFirst, IsLast, Result, cutoff_threshold_m);

			// 형태소 단위 분석 결과가 있으면
			if (num_result) 
			{
				// 메모리 해제
				ma_Free( restored_ej);
#ifdef DEBUG
				fprintf( stderr, "형태소 단위 분석 결과 있음\n");
#endif
//				num_morpheme_anal++;
				return MORPHEME;
			}
#ifdef DEBUG
			else
	 		{
				fprintf( stderr, "형태소 단위 분석 결과 없음\n");
			}
#endif
		}

		// 음절 단위 분석
		if (PsUnit & SYLLABLE_ANALYSIS) 
		{
//			num_syllable_anal_try++;

			num_result = ma_ExecS( rsc->s_rsc, restored_ej, Result,
							cutoff_threshold_s, BeamSize);

			// 음절 단위 분석 결과가 있으면
			if (num_result) 
			{
#ifdef DEBUG
			fprintf( stderr, "음절 단위 분석 결과 있음\n");
#endif

				// 메모리 해제
				ma_Free( restored_ej);

//				num_syllable_anal++;
				return SYLLABLE;
			}
#ifdef DEBUG
			else
		 	{
				fprintf( stderr, "음절 단위 분석 결과 없음\n");
			}
#endif
		}

		// 메모리 해제
		ma_Free( restored_ej);
	}

	// 여기까지 오는 경우는 분석 결과가 없을 때
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
	"·",
	"∼",
	"◇",
	"△",
	"▲",
	"→",
	"▷",
	"▶",
	"．",
	"＾",
	"ㆍ",
	"Ⅰ",
};

// 긴 어절을 분해하는 함수
// 완성형 코드에만 적용됨
// return value: 부분 어절의 수
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
	
	// 분리 문자의 빈도를 센다.
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
					case 0xA4: // ·
						char_freq[8]++;
						break;
					case 0xAD: // ∼ (공백)
						char_freq[9]++;
						break;
					case 0xDE: // ◇
						char_freq[10]++;
						break;
					case 0xE2: // △
						char_freq[11]++;
						break;
					case 0xE3: // ▲
						char_freq[12]++;
						break;
					case 0xE6: // →
						char_freq[13]++;
						break;
				}
				break;
			case 0xA2:
				if (second == 0xB9) // ▷
					char_freq[14]++;
				else if (second == 0xBA) // ▶
					char_freq[15]++;
				break;
			case 0xA3:
				if (second == 0xAE) // ．
					char_freq[16]++;
				else if (second == 0xDE) // ＾
					char_freq[17]++;
				break;
			case 0xA4:
				if (second == 0xFD) // ㆍ
					char_freq[18]++;
				break;
			case 0xA5:
				if (second == 0xB0) // Ⅰ
					char_freq[19]++;
				break;
		} // end of switch
	} // end of for
	
	for (i = 0; i < 20; i++)
	{
		if (char_freq[i] >= max) // 등호 사용 이유 : 동률인 경우 뒷문자를 선호
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
	
	// 시작 위치부터 구분자가 발견되면 다음 문자 위치부터로 설정하게 함
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
		if (*(from+delim_len) == 0) break; // 거의 마지막에 다다르면 여기서 중지
		to = strstr( from+delim_len, delim);
	}
	SubEjs[num_subej++] = strdup( from);
	
	return num_subej;
}
*/

///////////////////////////////////////////////////////////////////////////////
void GetFirstTag
(
	char	*FirstTag,	// [output] 첫 태그
	char	*MA_Result		// [input] 형태소 분석 결과
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
// 두 형태소 분석 결과를 합친다.
// Result <- Result + AddOn
int ma_IntegrateResult
(
	MAtRSC_M	*Rsc,
	MAtRESULTS	*Result,	// [in/output]
	MAtRESULTS	*AddOn,	// [input]
	int		IsLast,	// [input] 마지막 부분 어절인가?
	double	CutoffThreshold,
	int		TightCheck	// 연결 검사를 철저하게 할 것인가?
)
{
	int i;
	int j;
	
	MAtRESULTS	*new_results;
	double max_prob;
	double cur_prob;
	PROBtFST *rsc_trans = Rsc->transition;	// 전이 확률
	char cur_first_pos[8];
	double trans_prob;
	
	////////////////////////////////////////////////////////////////////////////////
	// Result가 빈 경우	
	if (Result->count == 0)
	{
		for (j = 0; j < AddOn->count; j++)
		{
			// 첫번째 품사 알아내기
			GetFirstTag( cur_first_pos, AddOn->mresult[j].str);
			
			// 어절 처음 태그와의 연결성 검사
			// 연결 불가능하면
			// P(첫품사 | t0)
			trans_prob = prob_GetFSTProb2( rsc_trans, cur_first_pos, (char *)BOW_TAG_1);
			
			if (TightCheck && (trans_prob <= PROBcLOG_ALMOST_ZERO))
			{
#ifdef DEBUG
				fprintf(stderr, "불가 %s -> %s\n", BOW_TAG_1, cur_first_pos);
#endif
				continue;
			}

			Result->mresult[Result->count].str = strdup( AddOn->mresult[j].str);
			Result->mresult[Result->count].prob = AddOn->mresult[j].prob + trans_prob;
			Result->count++;
		}
		
		if (Result->count == 0) return 0; // 모든 결과가 연결성 검사에서 실패하는 경우
		
		// 확률순으로 정렬
		qsort( Result->mresult, Result->count, sizeof( MAtRESULT), ma_CompareProb);
		
		return 1;
	}
	
	////////////////////////////////////////////////////////////////////////////////
	char *prev_ma_str;
	char *cur_ma_str;
	char *prev_last_pos;
	double trans_prob2 = 0.0; // 초기화
	int loop_quit = 0;
	
	new_results = (MAtRESULTS *)malloc( sizeof( MAtRESULTS)); //ma_New( 1);
	new_results->count = 0;
	
	max_prob = -LONG_MAX;
	
	for (i = 0; i < Result->count; i++)
	{
		/**/assert( Result->mresult[i].str != NULL);
		prev_ma_str = Result->mresult[i].str;
		
		// 마지막 품사 알아내기
		prev_last_pos = strrchr( prev_ma_str, '/')+1;
		//**/fprintf( stderr, "last pos = %s (%s)\n", prev_last_pos, prev_ma_str);
		
		for (j = 0; j < AddOn->count; j++)
		{
			/**/assert( AddOn->mresult[j].str != NULL);
			cur_ma_str = AddOn->mresult[j].str;
			
			// 첫번째 품사 알아내기
			GetFirstTag( cur_first_pos, cur_ma_str);

			//**/fprintf( stderr, "first pos = %s (%s)\n", cur_first_pos, cur_ma_str);
			
			// 전이확률
			trans_prob = prob_GetFSTProb2( rsc_trans, cur_first_pos, prev_last_pos);
			if (TightCheck && (trans_prob <= PROBcLOG_ALMOST_ZERO)) // 연결 불가능하면 // 확인해 볼 것!
			{
#ifdef DEBUG
				fprintf( stderr, "불가 %s -> %s\n", prev_last_pos, cur_first_pos);
#endif
				continue;
			}
			
			// 마지막 부분 어절이면 어절끝 태그와의 연결 가능성 여부 검사 및 전이 확률 계산
			if (IsLast)
			{
				// 마지막 품사 알아내기
				char *cur_last_pos = strrchr( cur_ma_str, '/')+1;
				//**/fprintf( stderr, "last pos = %s (%s)\n", cur_last_pos, cur_ma_str);
				
				// 전이확률 // P( EOW_TAG | 마지막태그)
				trans_prob2 = prob_GetFSTProb2( rsc_trans, (char *)EOW_TAG, cur_last_pos);
				if (TightCheck && (trans_prob2 <= PROBcLOG_ALMOST_ZERO)) // 연결 불가능하면 // 확인해 볼 것!
				{
#ifdef DEBUG
					fprintf( stderr, "불가 %s -> %s\n", cur_last_pos, (char *)EOW_TAG);
#endif
					continue;
				}
			}
			
			// 확률값
			cur_prob = Result->mresult[i].prob + AddOn->mresult[j].prob + trans_prob + trans_prob2;
			if (max_prob < cur_prob) max_prob = cur_prob;
			
			// 최고 확률을 갖는 결과보다 로그값의 차가 기준치 이상이면 종료
			if (max_prob - cur_prob > CutoffThreshold)
			{
#ifdef DEBUG
				fprintf( stderr, "cutoffed! in %s (%d) line\n", __FILE__, __LINE__);
#endif
				continue; // break은 위험함 (뒤에 나오는 것이 더 높은 확률을 가질 수도 있으므로)
			}

			new_results->mresult[new_results->count].prob = cur_prob;
			
			// 메모리 할당
			char *str = (char *)malloc( strlen( prev_ma_str) + strlen( cur_ma_str) + 2);
			
			// 문자열 합침
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
	
	// 기존 분석 결과 삭제
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
		return 0; // 모든 결과가 연결성 검사에서 실패하는 경우
	}

	// 정렬
	qsort( new_results->mresult, new_results->count, sizeof( MAtRESULT), ma_CompareProb);
	
	max_prob = new_results->mresult[0].prob;

	// 기존 분석 결과 <- 새로운 분석 결과 복사
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
	
	// 새로운 분석 결과 삭제
	ma_Free( new_results);
	
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
int ma_Exec            
(                      
	const char		*Eojeol,	// [input] 어절
	void			*Rsc,		// [input] 리소스
	int			PsUnit,	// [input] 처리 단위 (어절,형태소,음절)
	MAtRESULTS		*Result	// [output] 형태소 분석 결과
)
{
	int ret;
	int tight_check = 1;
	
	// 초기화
	Result->count = 0;

#ifdef DEBUG
	fprintf( stderr, "---------------\n%s\t(line %d in %s)\n", Eojeol, __LINE__, __FILE__);
#endif

	// 어절 단위 분석
	if (PsUnit & EOJEOL_ANALYSIS) 
	{
		int num_result;
		
//		/**/num_eojeol_anal_try++;
		num_result = ma_ExecE( ((MAtRSC *)Rsc)->e_rsc, Eojeol, Result);
	 
		// 어절 단위 분석 결과가 있으면
		if (num_result) 
		{
//			/**/num_eojeol_anal++;
#ifdef DEBUG
			fprintf( stderr, "어절 단위 분석 결과 있음\n");
#endif
			return Result->count;
		}
#ifdef DEBUG
		else
	 	{
			fprintf( stderr, "어절 단위 분석 결과 없음\n");
		}
#endif
	}
	
	int len = strlen( Eojeol);
	char *subEjs[len];
	int ctypes[len];
	int num_subejs;
	int i;
	MAtRESULTS *ma_results = NULL;	// 형태소 분석 결과 저장
	
	// 어절을 문자유형에 따라 분해
	num_subejs = SplitWord( Eojeol, len, subEjs, ctypes);
	
#ifdef DEBUG
	fprintf( stderr, "# of sub-Eojeols = %d\n", num_subejs);
#endif
	
	if (num_subejs == 0) return 0;

	// 부분 어절의 수가 많지 않으면 통째로 분석 시도	
	if (num_subejs < 10)
	{
		ret = morphological_analysis( Rsc, Eojeol, 1, 1, 10, 10, 15, PsUnit, Result);
		
		// 분석 결과가 있으면 종료
		if (ret != 0)
		{
#ifdef DEBUG
			fprintf( stderr, "전체 어절 분석 결과\n");
			ma_PrintResult( stderr, Eojeol, Result);
#endif

			for (i = 0; i < num_subejs; i++) free( subEjs[i]);
			return Result->count;
		}
#ifdef DEBUG
			fprintf( stderr, "전체 어절 분석 결과 없음\n");
#endif
	}
	
	ma_results = ma_New( num_subejs);
	
	assert( ma_results != NULL);
	
	// 부분 어절의 수가 임계값 이상일 때는 품사 전이 가능성이 없어도 cutoff 시키지 않음
	//if (num_subejs >= 10) tight_check = 0;
	tight_check = 0;
	
	// 둘 이상의 부분 어절이 있는 경우
	for (i = 0; i < num_subejs; i++)
	{
		char *subej = subEjs[i];
#ifdef DEBUG
		fprintf( stderr, "sub-ej[%d]: %s (%s)\n", i, subej, UHCcCTYPE[ctypes[i]]);
#endif

		switch (ctypes[i])
		{
			case UHC_HANGEUL: // 한글의 빈도가 가장 높으므로 가장 먼저 실행
			case ASC_PUNCT:
			case UHC_SYMBOL:
				ret = morphological_analysis( Rsc, subej, i == 0 ? 1 : 0, i == (num_subejs-1) ? 1 : 0, 10, 10, 15, PsUnit, &ma_results[i]);
				
				if (ret == 0)
				{
					if (ctypes[i] == UHC_SYMBOL || ctypes[i] == ASC_PUNCT) // 기호 미분석은 SW
						__AssignResult( &ma_results[i], subej, "SW");
					else __AssignResult( &ma_results[i], subej, "NA"); // 한글 미분석은 NA
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
				// 아래아이면 (가운뎃점과 모양이 흡사해서 가운뎃점으로 사용하는 경우가 있음)
				if (strlen( subej) == 2 && (unsigned char)subej[0] == 0xA4 && (unsigned char)subej[1] == 0xFD)
				{
					__AssignResult( &ma_results[i], subej, "SP");
				}
				else __AssignResult( &ma_results[i], subej, "SW");
				break;

			default: // 여기로 온 경우는 오류
				assert( 0);
				__AssignResult( &ma_results[i], subej, "NA");
				break;
		} // end of switch
		
#ifdef DEBUG
		// 부분 분석 결과 출력
		fprintf( stderr, "부분 어절 분석 결과\n");
		ma_PrintResult( stderr, subej, &ma_results[i]);
#endif
		// 분석 결과 통합
		ret = ma_IntegrateResult( ((MAtRSC *)Rsc)->m_rsc, Result, &ma_results[i], i == (num_subejs-1) ? 1 : 0, 10, tight_check);
		
		if (ret == 0)
		{
			__AssignResult( Result, Eojeol, "NA");
			break;
		}
	} // end of for
	
	for (i = 0; i < num_subejs; i++) free( subEjs[i]);
	
	// 분석 결과에 대한 메모리 해제
	ma_FreeAll( ma_results, num_subejs);

	return Result->count;
}
