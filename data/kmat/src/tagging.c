//#define DEBUG

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "kmat.h"
#include "tagging.h"
#include "kmat_def.h"

#include "hsplit.h"
#include "probtool.h"
#include "get_morph_tag.h"
#include "viterbi.h"
#include "strvector.h"

#define INTER_TRANS_FST     0
#define INTER_TRANS_PROB    1
#define INTRA_TRANS_FST     2
#define INTRA_TRANS_PROB    3
//#define INTER_TRANS_EJ_PRB  4

const char *tag_RSC_FILES[] = {
	"INTER_TRANS.fst",		// 0
	"INTER_TRANS.prob",	 // 1
	"INTRA_TRANS.fst",		// 2
	"INTRA_TRANS.prob",		// 3
//	"INTER_TRANS_EJ.prb", // 2
	NULL,
};

// 전역 변수
// 화일명을 포함한 경로
char tag_RSC_FILES_PATH[10][100]; // 10

///////////////////////////////////////////////////////////////////////////////
void tag_Close
(
	void	*Rsc
)
{
	TAGtRSC *rsc = (TAGtRSC *)Rsc;
	
	prob_CloseFST( rsc->inter_transition);
	prob_CloseFST( rsc->intra_transition);
	
	if (rsc)
	{
		free( rsc);
		rsc = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
void *tag_Open
(
	char	*RscPath
)
{
	TAGtRSC *rsc; // 리소스
	int i;
	char rsc_path[300];
	
	strcpy( rsc_path, RscPath);
		
	// 리소스 경로 설정 (경로 + 파일명)
	for (i = 0; tag_RSC_FILES[i]; i++)
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
			sprintf( tag_RSC_FILES_PATH[i], "%s%s", rsc_path, tag_RSC_FILES[i]);
		}
		else
		{
			sprintf( tag_RSC_FILES_PATH[i], "%s", tag_RSC_FILES[i]);
		}
	}
	
	// 리소스
	rsc = (TAGtRSC *) malloc( sizeof( TAGtRSC));

	// 확률을 입력
	// 외부전이 확률
	rsc->inter_transition = prob_OpenFST( tag_RSC_FILES_PATH[INTER_TRANS_FST], tag_RSC_FILES_PATH[INTER_TRANS_PROB]);
	
	assert( rsc->inter_transition != NULL);

	// 내부전이 확률
	rsc->intra_transition = prob_OpenFST( tag_RSC_FILES_PATH[INTRA_TRANS_FST], tag_RSC_FILES_PATH[INTRA_TRANS_PROB]);
	assert( rsc->intra_transition != NULL);
	
	return (void *)rsc;
}

///////////////////////////////////////////////////////////////////////////////
// 확률적 형태소 분석
// 리턴값 : 0 = 분석결과 없음, 1 = 분석 결과 있음
// input_ej : 입력 어절
// analyzed_result : 분석 결과 (확률값은 로그)
int tag_Exec
(
	void		*Rsc,				// [input] 리소스
	MAtRESULTS	*MorphAanlyzedResult,	// [input] 형태소 분석 결과
	int		NumWord,			// [input] 어절의 수
	int		*StateSeq			// [output] 태깅 결과 (형태소 분석 결과 번호)
) 
{
	TAGtRSC *rsc = (TAGtRSC *)Rsc;


	ViterbiSearch2gram( rsc->intra_transition,
					rsc->inter_transition,
					MorphAanlyzedResult,
					NumWord,
					StateSeq);

	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// 태깅 결과 출력
void tag_PrintResult
(
	FILE		*Fp,				// [output]
	char		**Words,			// [input] 문장내 어절
	MAtRESULTS	*MorphAanlyzedResult,	// [input] 형태소 분석 결과
	int		NumWord,			// [input] 어절의 수
	int		*StateSeq			// [input] 태깅 결과 (형태소 분석 결과의 번호)
)
{
	int i;
	
	// 고대 스타일
//	if ( !output_style) 
	{
		// 각 어절에 대해
		for (i = 0; i < NumWord; i++)
		{
#ifdef DEBUG
			fprintf( Fp, "[%d]\t", StateSeq[i+1]);
#endif
			// 원어절, 분석결과
			fprintf( Fp, "%s\t%s\n", Words[i], MorphAanlyzedResult[i+1].mresult[StateSeq[i+1]].str); // 어절
		}
	}

	// 세종계획 스타일
//	else 
//	{
//		int morph_num = 0; // 어절 내의 형태소 수
//		int j = 0;
//		SVtSTRVCT *morphs;
//		SVtSTRVCT *tags;
//	
//		morphs = sv_New();
//		tags = sv_New();
//
//		// 어절마다
//		for (i = 0; i < NumWord; i++) 
//		{
//			// 원어절
//			fprintf( Fp, "%s\t", Words[i]);
//
//			// 분석결과
//			morph_num = GetMorphTag( MorphAanlyzedResult[i+1].mresult[StateSeq[i+1]].str, 
//							DELIMITER,
//							morphs,
//							tags);
//
//			// 형태소마다
//			for (j = 0; j < morph_num; j++) 
//			{
//				if (j) 
//				{
//					fprintf( Fp, " + %s%c%s", morphs->strs[j], DELIMITER, tags->strs[j]); 
//				}
//				else 
//				{
//					fprintf( Fp, "%s%c%s", morphs->strs[j], DELIMITER, tags->strs[j]);
//				}
//			}
//			fprintf( Fp, "\n");
//		}
//		sv_Free( morphs);
//		sv_Free( tags);
//	}
}
