#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "kmat.h"
#include "kmat_def.h" // BOW_TAG_1
#include "ma.h"
#include "tagging.h"

typedef struct
{
	MAtRSC	*ma_rsc;	// 형태소 분석
	TAGtRSC	*tag_rsc;	// 태깅
} KMATtRSC;

////////////////////////////////////////////////////////////////////////////////
void *kmat_Open
(
	char	*RSC_Path
)
{
	KMATtRSC *rsc = (KMATtRSC *) malloc( sizeof( KMATtRSC));
	assert( rsc != NULL);
	
	// 7 = EOJEOL_ANALYSIS|MORPHEME_ANALYSIS|SYLLABLE_ANALYSIS
	rsc->ma_rsc = ma_Open( RSC_Path, 7);
	assert( rsc->ma_rsc != NULL);

	rsc->tag_rsc = tag_Open( RSC_Path);
	assert( rsc->tag_rsc != NULL);
	
	return rsc;
}

////////////////////////////////////////////////////////////////////////////////
void kmat_Close
(
	void	*Rsc
)
{
	KMATtRSC *rsc = (KMATtRSC *) Rsc;
	
	if (rsc == NULL) return;
	
	ma_Close( rsc->ma_rsc, 7);
	tag_Close( rsc->tag_rsc);
	
	free( rsc);
	rsc = NULL;
}

////////////////////////////////////////////////////////////////////////////////
void kmat_Exec
(
	void		*Rsc,		// [input] 리소스
	char		**Words,	// [input] 형태소 분석 결과
	int		NumWord,	// [input] 어절의 수
	char		**Results	// [output] 태깅 결과
)
{
	MAtRSC *ma_rsc = ((KMATtRSC *)Rsc)->ma_rsc;
	TAGtRSC *tag_rsc = ((KMATtRSC *)Rsc)->tag_rsc;
	
	MAtRESULTS *ma_results = NULL;	// 형태소 분석 결과 저장
	int state_sequence[KMATcMAX_EJ_NUM]; // 품사 태깅 결과
	int i;
	
	// 메모리 할당
	ma_results = ma_New( NumWord+1); // +1 : 문장 시작 기호
	
	// 형태소 분석 결과 초기화
	// 0번째를 채운다.
	//init_morph_analyzed_result( &ma_results[0]);
	ma_results->count = 1;
	ma_results->mresult[0].str = strdup( BOW_TAG_1);
	ma_results->mresult[0].prob = 1.0;
	
	// 각 어절에 대한 형태소 분석
	for (i = 0; i < NumWord; i++)
	{
		ma_Exec( Words[i], ma_rsc, EOJEOL_ANALYSIS|MORPHEME_ANALYSIS|SYLLABLE_ANALYSIS, &ma_results[i+1]);
	}

	// 품사태깅
	tag_Exec( tag_rsc, ma_results, NumWord, state_sequence);
	
	// 결과 세팅
	for (i = 0; i < NumWord; i++)
	{
		Results[i] = strdup( ma_results[i+1].mresult[state_sequence[i+1]].str);
		
		//Results[i] = Postprocessing_by_userdic( ma_results[i+1].mresult[state_sequence[i+1]].str);
	}
	
	// 분석 결과에 대한 메모리 해제
	ma_FreeAll( ma_results, NumWord+1);
}

////////////////////////////////////////////////////////////////////////////////
void kmat_PrintResult
(
	FILE		*Fp,		// [output]
	char		**Words,	// [input] 문장내 어절
	int		NumWord,	// [input] 어절의 수
	char		**Results	// [output] 태깅 결과
)
{
	int i;
	
	for (i = 0; i < NumWord; i++)
	{
		fprintf( Fp, "%s\t%s\n", Words[i], Results[i]);
	}
}
