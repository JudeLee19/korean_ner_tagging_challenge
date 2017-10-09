//#define DEBUG

#include <stdio.h>
#include <math.h>	 // log
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "ma.h"
#include "kmat_def.h"
#include "cyk.h"
#include "hsplit.h"
#include "probtool.h"
#include "phonetic_recover.h"
#include "triangular_matrix.h"
#include "unit_conversion.h"
#include "dafst.h"
#include "bin2txt.h"

///////////////////////////////////////////////////////////////////////////////
void ma_CloseM
(
	MAtRSC_M	*Rsc
)
{
	if (Rsc == NULL) return;
	
	prob_CloseFST( Rsc->lexical); //fst_probability_close( lexical);
	prob_CloseFST( Rsc->transition);
	prob_CloseFST( Rsc->morph);
	
	free( Rsc);
	Rsc = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// 리소스 열기
MAtRSC_M *ma_OpenM
(
	char	*LEXICAL_FST_Path,
	char	*LEXICAL_INFO_Path,
	char	*LEXICAL_PROB_Path,
	char 	*TRANSITION_FST_Path,
	char	*TRANSITION_PROB_Path,
	char	*M_MORPH_FST_Path,
	char	*M_MORPH_PROB_Path
)
{
	MAtRSC_M *rsc = (MAtRSC_M *) malloc( sizeof( MAtRSC_M));
	if (rsc == NULL) return NULL;
	
	// 확률을 입력
	// 어휘 확률
	rsc->lexical = prob_OpenWithInfoFST( LEXICAL_FST_Path, LEXICAL_INFO_Path, LEXICAL_PROB_Path);
	assert( rsc->lexical != NULL);
	
	// 전이 확률
	rsc->transition = prob_OpenFST( TRANSITION_FST_Path, TRANSITION_PROB_Path);
	assert( rsc->transition != NULL);

	// 확률
	rsc->morph = prob_OpenFST( M_MORPH_FST_Path, M_MORPH_PROB_Path);
	assert( rsc->morph != NULL);
	
	return rsc;
}

///////////////////////////////////////////////////////////////////////////////
// 모든 부분 문자열을 미리 사전에서 찾는다.
// 추후에 효율적으로 수정할 필요가 있음
static int get_all_possible_pos_from_substring
(
	int 		len,
	char 		*SrcStr,
	PROBtFST 	*lexical, 
	MAtRESULTS	*SubstrInfo	// [output]
)
{
	int i, j, k;

	char str[MAX_WORD];
	int n, index, m;
	int count;

	// 반드시 초기화해야 함
	// 음운복원 후 같은 변수에 다시 저장을 하기 때문 (초기화를 하지 않으면 이전 데이타로 인해 문제 발생)

	// 부분 문자열 저장
	for (k = i = 0; i < len; i++) 
	{
		for (j = i; j < len; j++, k++) 
		{
			strncpy( str, SrcStr+i*2, (j-i+1)*2);
			str[(j-i+1)*2] = 0;
//#ifdef DEBUG
//			fprintf(stderr, "[%d] %s\n", k, str);
//#endif
			// 부분 문자열을 사전에서 찾는다.
			if ((n = fst_String2Hash( lexical->fst, str, &index)) != (-1))
			{
				// 있으면
				// 취할 수 있는 모든 품사를 찾는다.
				for (m = 0; m < index; m++)
				{
					count = SubstrInfo[k].count;
					SubstrInfo[k].mresult[count].str = strdup( b2t_GetString( lexical->info, n+m));
					SubstrInfo[k].mresult[count].prob = 0.0; // log(1)
					SubstrInfo[k].count++;
				}
			} // end of if
		} // end of for
	} // end of for

	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// result_m에 저장된 결과를 정리하여 analyzed_result_m에 저장
// 리턴값 : 분석 결과의 수
static int arrange_result_m
(
	MAtRESULTS	*Result_in,
	MAtRESULTS	*Result_out,
	double	CutoffThreshold
)
{
	int i;
	
	// 결과 변환
	char result_str[MAX_WORD]; // FIL을 제거한 결과

	if (Result_in->count < 1) return 0; // 결과가 없으면 종료

	// 확률 normalization
	double max_prob = Result_in->mresult[0].prob; // 최고 확률값
	
	Result_out->count = 0; // 초기화
	//assert( Result_out->count == 0);

	// 모든 분석 결과에 대해
	// res_itr->first : 확률값
	// res_itr->second : 형태소 분석결과
	for (i = 0;	i < Result_in->count; i++)
	{
		
		assert( max_prob >= Result_in->mresult[i].prob);
		
		// cut-off가 가능한지 검사
		// 최고 확률을 갖는 결과보다 로그값의 차가 기준치이상이면 종료
		if (CutoffThreshold > 0 && (max_prob - Result_in->mresult[i].prob) > CutoffThreshold) 
		{
			// 여기서부터 마지막까지는 저장할 필요없음 (나머지 결과의 확률값은 더 작으므로)
			break;
		}
		
		if (strstr( Result_in->mresult[i].str, "/NA") != NULL) continue; // 분석 결과에 NA가 있으면 안됨

		// 생성된 태그열의 정당성 검사
/*	복구해야함	if ( !(num_morph = check_morpheme_result( Result_in->mresult[i].str, str_syl_tag_seq, DELIMITER))) 
		{
			continue; // do nothing
		}
*/		
		// 여기까지 온 경우는 저장해야 함
		convert_str_origin_array( Result_in->mresult[i].str, result_str); // 결과에서 FIL을 제거
		Result_out->mresult[Result_out->count].str = strdup( result_str );
		Result_out->mresult[Result_out->count].prob = Result_in->mresult[i].prob; // 로그 확률
		Result_out->count++;
	} // end of for

	return Result_out->count; // 분석된 결과의 수
}

///////////////////////////////////////////////////////////////////////////////
// 확률적 형태소 분석 (형태소 레벨)
int ma_ExecM
(
	MAtRSC_M	*Rsc,
	MAtRESULTS	*RestoredEJ,
	int		IsFirst,	// 어절 시작부인가?
	int		IsLast,	// 어절 종료부인가?
	MAtRESULTS	*Result, // [output]
	double	CutoffThreshold
)
{
	int num_syll; // 음절 수
	t_TAB position; // 문자열의 위치값
	
	MAtRESULTS *substr_info; // 부분 문자열 정보
	
	int num_cell; // 삼각표의 셀의 수
	int i, j, k;
	
	MAtRESULTS *result_m;
	char **sub_str; // 부분 문자열 저장
	
	int num_result = 0;
	
	// 메모리 할당		
	result_m = (MAtRESULTS *) malloc( sizeof (MAtRESULTS));
	assert( result_m != NULL);
	
	result_m->count = 0;
	
	double max_restored_prob = RestoredEJ->mresult[0].prob; // 첫 음운 복원 확률

	// 모든 복원된 어절에 대해 반복
	int ej_num = 0; // 복원된 어절의 번호
	for (i = 0; i < RestoredEJ->count; i++, ej_num++)
	{
		// 문자열 길이 (음절 수)
		num_syll = (int) strlen( RestoredEJ->mresult[i].str) / 2; 

#ifdef DEBUG
		fprintf( stderr, "어절 크기 = %d (음절)\n", num_syll);
#endif
		
		// 음절 수가 15보다 많은 어절은 첫번째 복원된 어절에 대해서만 실행
		if (num_syll >= 15) 
		{
			if (i) break;
		}

		// 음운 복원 확률값이 너무 작은 경우나 0인 경우에는 분석하지 않는다.
		if ( RestoredEJ->mresult[i].prob <= PROBcLOG_ALMOST_ZERO 
				 || (CutoffThreshold > 0.0 && (max_restored_prob - RestoredEJ->mresult[i].prob) > 3/*CutoffThreshold*/) ) 
		{
#ifdef DEBUG
			if (RestoredEJ->mresult[i].prob <= PROBcLOG_ALMOST_ZERO) 
				fprintf(stderr, "복원 확률값 = 0\n");
			else fprintf(stderr, "너무 작은 복원 확률값\n");
#endif
			break;
		}

		if (num_syll > 30) // 30음절 초과이면 CYK 알고리즘이 너무 비효율적이므로 분석 하지 않음
		{
#ifdef DEBUG
			fprintf( stderr, "형태소 단위 분석 포기\n");
#endif
			continue;
		}

		// 초기화
		setpos( &position, 0, num_syll); // 위치 지정
		
		num_cell = TabNum( num_syll);
		
		substr_info = (MAtRESULTS *)malloc( sizeof( MAtRESULTS) * num_cell);
		assert( substr_info != NULL);
		
		sub_str = (char **)malloc( sizeof( char *) * num_cell);
		assert( sub_str != NULL);
		
		// 초기화
		for (j = 0; j < num_cell; j++)
		{
			substr_info[j].count = 0;
		}
		
		// 부분 문자열 저장
		// 출력 : sub_str
		GetAllSubstring( RestoredEJ->mresult[i].str, num_syll, sub_str);
		
		// 부분 문자열 저장
		// 모든 부분 문자열을 미리 사전에서 찾는다.
		// 출력 : substr_info (각 부분 문자열이 취할 수 있는 품사와 확률의 열 저장)
		get_all_possible_pos_from_substring( num_syll, 
								RestoredEJ->mresult[i].str, 
								Rsc->lexical,
								substr_info);

		// 음운복원된 어절 출력
#ifdef DEBUG
		fprintf(stderr, "복원된 어절 : %s\t%e\n", RestoredEJ->mresult[i].str, RestoredEJ->mresult[i].prob);
#endif

		// 형태소 분석 모듈
		cyk_ExecM( Rsc,
				substr_info, 
				num_cell,
				ej_num, // 어절 번호
				sub_str,
				num_syll, 
				RestoredEJ->mresult[i].prob, // 로그 확률 (초기값)
				CutoffThreshold,
				log( pow( 1.0e-03, num_syll)),
				result_m);

		// 메모리 해제
		for (j = 0; j < num_cell; j++)
		{
			for (k = 0; k < substr_info[j].count; k++)
			{
				free( substr_info[j].mresult[k].str);
			}
		}
		free( substr_info);
		
		for (j = 0; j < num_cell; j++)
		{
			free( sub_str[j]);
		}
		free( sub_str);

	} // end of for
	
#ifdef DEBUG
	fprintf( stderr, "형태소 단위 분석 결과의 수 = %d\n", result_m->count);
#endif

	if (result_m->count > 0) // 형태소 단위 분석 결과가 있으면
	{
		// 정렬
		qsort( result_m->mresult, result_m->count, sizeof( MAtRESULT), ma_CompareProb);

		// 결과 정리
		num_result = arrange_result_m( result_m, Result, CutoffThreshold);
	}
	
	// 메모리 해제
	ma_Free( result_m);

	return num_result;
}
