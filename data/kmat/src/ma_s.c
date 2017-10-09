//#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
//#include <math.h> // exp

#include "ma.h"
#include "dafst.h"
#include "bin2txt.h"
#include "hsplit.h"
#include "phonetic_recover.h"
#include "bfs.h"
#include "probtool.h"
#include "kmat_def.h"
#include "kmat.h"
#include "unit_conversion.h"

///////////////////////////////////////////////////////////////////////////////
void ma_CloseS
(
	MAtRSC_S *Rsc
)
{
	if (Rsc == NULL) return;
	
	prob_CloseFST( Rsc->tag_s); //fst_probability_close( tag_s);
	prob_CloseFST( Rsc->syllable_s);

	if (Rsc->s_transition_fst) fst_Close( Rsc->s_transition_fst); // FST
	
	if (Rsc->syllable_tag_fst) fst_Close( Rsc->syllable_tag_fst);
	
	b2t_Close( Rsc->syllable_tag_info);
	
	free( Rsc);
	Rsc = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// 리소스 열기
MAtRSC_S *ma_OpenS
(
	char	*TAG_S_FST_Path,
	char	*TAG_S_PROB_Path, 
	char	*SYLLABLE_S_FST_Path,
	char	*SYLLABLE_S_PROB_Path,
	char	*S_TRANSITION_FST_Path,
	char	*SYLLABLE_TAG_FST_Path,
	char	*SYLLABLE_TAG_INFO_Path
)
{
	MAtRSC_S *rsc = (MAtRSC_S *) malloc( sizeof( MAtRSC_S));
	if (rsc == NULL) return NULL;
	
	rsc->tag_s = prob_OpenFST( TAG_S_FST_Path, TAG_S_PROB_Path);
	assert( rsc->tag_s != NULL);

	rsc->syllable_s = prob_OpenFST( SYLLABLE_S_FST_Path, SYLLABLE_S_PROB_Path);
	assert( rsc->syllable_s != NULL);

	fprintf( stderr, "\tLoading syllable transition FST.. [%s]", S_TRANSITION_FST_Path);
	rsc->s_transition_fst = fst_Open( S_TRANSITION_FST_Path, NULL);
	if (rsc->s_transition_fst == NULL) 
	{
		fprintf( stderr, "Load failure [%s]\n", S_TRANSITION_FST_Path);
		return 0;
	}
	fprintf( stderr, "\t[done]\n");
	
	fprintf( stderr, "\tLoading syllable-tag FST.. [%s]", SYLLABLE_TAG_FST_Path);
	rsc->syllable_tag_fst = fst_Open( SYLLABLE_TAG_FST_Path, NULL);
	if (rsc->syllable_tag_fst == NULL) 
	{
		fprintf( stderr, "Load failure [%s]\n", SYLLABLE_TAG_FST_Path);
		return 0;
	}
	fprintf( stderr, "\t[done]\n");
	
	fprintf( stderr, "\tLoading syllable-tag information.. [%s]", SYLLABLE_TAG_INFO_Path);
	if ((rsc->syllable_tag_info = b2t_Open( SYLLABLE_TAG_INFO_Path)) == NULL)
	{
		fprintf( stderr, "b2t_Open error [%s]\n", SYLLABLE_TAG_INFO_Path);
		return NULL;
	}
	fprintf( stderr, "\t[done]\n");	
	
	return rsc;
}

///////////////////////////////////////////////////////////////////////////////
// result_s에 저장된 결과를 정리하여 analyzed_result_s에 저장
// 리턴값 : 분석 결과의 수
static int arrange_result_s
(
	MAtRESULTS	*Result_in,
	MAtRESULTS	*Result_out, // [output]
	double	CutoffThreshold
)
{

	char result_str[MAX_WORD]; // FIL을 제거한 결과
	int i;
	
	Result_out->count = 0; // 초기화
	
	if (Result_in->count < 1) return 0; // 결과가 없으면 종료

	double max_prob = Result_in->mresult[0].prob; // 최고 확률값

	// 반복
	for (i = 0; i < Result_in->count; i++)
	{
		if (CutoffThreshold > 0)	// 이 값이 0 이상인 경우에만 사용함
		{
			// cut-off가 가능한지 검사
			// 최고 확률을 갖는 결과보다 로그값의 차가 기준치이상이면 종료
			if (max_prob - Result_in->mresult[i].prob > CutoffThreshold) 
			{
				// 여기서부터 마지막까지는 저장할 필요없음 (나머지 결과의 확률값은 더 작으므로)
				break;
			}
		}
		
		if (strstr( Result_in->mresult[i].str, "/NA") != NULL) continue; // 분석 결과에 NA가 있으면 안됨
		
		// 여기까지 온 경우는 저장해야 함
		convert_str_origin_array( Result_in->mresult[i].str, result_str); // 결과에서 FIL을 제거
		Result_out->mresult[Result_out->count].str = strdup( result_str );
		Result_out->mresult[Result_out->count].prob = Result_in->mresult[i].prob; // 로그 확률
		Result_out->count++;
	}

	return Result_out->count; // 분석된 결과의 수
}

///////////////////////////////////////////////////////////////////////////////
// 확률적 형태소 분석 (음절 단위)
int ma_ExecS
(
	MAtRSC_S	*Rsc,
	MAtRESULTS	*RestoredEJ,	// 음운 복원된 어절
	MAtRESULTS	*Result,		// [output]
	double	CutoffThreshold,
	int		BeamSize
)
{
	char rest_ej[MAX_WORD]; // FIL 제거된 (음운복원된) 어절
	char splitchar[MAX_WORD][3]; // 어절에서 분리된 개개의 문자를 모두 2바이트로 변환하여 저장
	int num_splitchar = 0;

	char morph_result[MAX_WORD]; // 결과 저장
	int i, j;

	MAtRESULTS *result_s;

	result_s = (MAtRESULTS *) malloc( sizeof (MAtRESULTS));
	assert( result_s != NULL);
	
	result_s->count = 0;	

	double max_restored_prob = RestoredEJ->mresult[0].prob; // 첫 음운 복원 확률
	
	int ej_num = 0; // 복원된 어절의 번호

#ifdef DEBUG	
	fprintf( stderr, "복원된 어절의 수 = %d (line %d in %s)\n", RestoredEJ->count, __LINE__, __FILE__);
	fprintf( stderr, "CutoffThreshold = %f\n", CutoffThreshold);
#endif
	// 모든 복원된 어절에 대해 반복
	for (i = 0; i < RestoredEJ->count; i++, ej_num++)
	{
		convert_str_origin_array( RestoredEJ->mresult[i].str, rest_ej); // FIL 제거
		num_splitchar = split_by_char( rest_ej, &splitchar[2]);

		// 긴 어절 처리
/*		if (num_splitchar >= 40) {
			fprintf(stderr, "Error: too long word! [%s]\n", rest_ej);
			continue;
		}
*/
		// 음절 수가 15보다 많은 어절은 첫번째 복원된 어절에 대해서만 실행
		if (i && num_splitchar >= 15) 
		{
#ifdef DEBUG	
			fprintf( stderr, "복원된 어절의 길이 초과 (%d) (line %d in %s)\n", num_splitchar, __LINE__, __FILE__);
#endif
			break;
		}


		// 음운 복원 확률값이 너무 작은 경우나 0인 경우에는 분석하지 않는다.
		if ((max_restored_prob - RestoredEJ->mresult[i].prob) > CutoffThreshold
			|| RestoredEJ->mresult[i].prob <= PROBcLOG_ALMOST_ZERO) 
		{
			break;
		}
#ifdef DEBUG
		fprintf(stderr, "복원된 어절 = %s\n음절 수 = %d\n어절 번호 = %d\n", rest_ej, num_splitchar, ej_num);
#endif
		// 초기화
		strcpy( splitchar[0], BOW_SYL_2);	 // 어절 시작 음절
		strcpy( splitchar[1], BOW_SYL_1);	 // 어절 시작 음절

		strcpy( splitchar[num_splitchar+2], EOW_SYL);	 // 어절 끝 음절

		///////////////////////////////////////////////////////////////////////////////
		MAtRESULTS *result_bfs;
		
		result_bfs = (MAtRESULTS *)malloc( sizeof( MAtRESULTS));
		assert( result_bfs != NULL);
		
		result_bfs->count = 0;

		// 이전 두 상태를 고려함 (trigram)
		// 이전 한 상태를 고려함 (bigram) -> trigram 모델로 재계산
		bigram_BFS( Rsc, ej_num, splitchar, num_splitchar,
				RestoredEJ->mresult[i].prob, // 확률
				CutoffThreshold,
				BeamSize,
				result_bfs); //결과저장

#ifdef DEBUG
		fprintf( stderr, "result_bfs->count = %d\n", result_bfs->count);
#endif
		// 모든 가능한 결과에 대해
		// 마지막보다 하나 더 (어절 끝 음절까지)
		for (j = 0; j < result_bfs->count; j++)
		{
			if (j >= BeamSize) 
				break;
			
			char *tags[1024];
			int num_tags = 0;

#ifdef DEBUG			
			fprintf( stderr, "%s\n", result_bfs->mresult[j].str);
#endif
			num_tags = getSyllableTags( result_bfs->mresult[j].str, tags); // 태그를 하나씩 얻어냄
		
			// 결과 생성 (음절 단위로 태깅된 것을 형태소 단위 결과로 변환)
			if ( !ConvertSyll2Morph( splitchar, tags, num_splitchar, DELIMITER, morph_result)) 
			{
				// 음절->형태소 결과 실패
				assert( 0);
				continue; // 에러 발생한 경우
			}
#ifdef DEBUG
			fprintf( stderr, "%s\t%e\n", morph_result, result_bfs->mresult[j].prob); // 분석 결과
#endif		
			// 결과 저장 (확률 + 형태소/품사 열)	
			result_s->mresult[result_s->count].str = strdup( morph_result);
			result_s->mresult[result_s->count].prob = result_bfs->mresult[j].prob;
			result_s->count++;

		} // end of for j
		
		// 메모리 해제
		ma_Free( result_bfs);
	} // end of for i

	// 정렬
	qsort( result_s->mresult, result_s->count, sizeof( MAtRESULT), ma_CompareProb);

	// 결과 정리
	int num_result = arrange_result_s( result_s, Result, CutoffThreshold);
	
	// 메모리 해제
	ma_Free( result_s);

	return num_result;
}
