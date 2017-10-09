//#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <limits.h> // LONG_MAX : 2147483647
#include <assert.h>
#include <string.h>

#include "ma.h"
#include "kmat_def.h"
#include "bin2txt.h"
#include "dafst.h"
#include "probtool.h"
#include "strvector.h"
#include "unit_conversion.h"

///////////////////////////////////////////////////////////////////////////////
// 현재 태그와 이전 태그와의 연결성 검사
// 리턴값 : 1 = 연결 가능, 0 = 연결 불가능
static int check_connectivity
(
	const char	*CurTag,
	const char	*PrevTag
)
{
	// B_STYLE 일때
	if (CurTag[0] == 'I') 
	{
		if (strcmp( &CurTag[2], &PrevTag[2]) != 0) 
			return 0;
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// 현재 태그와 이전 태그와의 연결성 검사
// 리턴값 : 1 = 연결 가능, 0 = 연결 불가능
static int check_connectivity2
(
	const char	*CurTag,
	const char	*PrevTag
)
{
	 // 예) 이전 태그가 SL일때(시작이거나 중간이거나 상관없이) 다음 태그가 SL의 시작이면 결합 불가

	 if (strcmp( &PrevTag[2], "SL") == 0 && strcmp( CurTag, "B-SL") == 0) // 외국어
		 return 0;

	 if (strcmp( &PrevTag[2], "SH") == 0 && strcmp( CurTag, "B-SH") == 0) // 한자
		 return 0;

	 if (strcmp( &PrevTag[2], "SN") == 0 && strcmp( CurTag, "B-SN") == 0) // 숫자
		 return 0;

	 if (strcmp( &PrevTag[2], "SE") == 0 && strcmp( CurTag, "B-SE") == 0) // 줄임표
		 return 0;

	 if (strcmp( &PrevTag[2], "SO") == 0 && strcmp( CurTag, "B-SO") == 0) // 붙임표
			 return 0;

	 return 1;
}

///////////////////////////////////////////////////////////////////////////////
void bigram_BFS
(
	MAtRSC_S		*Rsc,			// 리소스
	int			EjIndex,		// 어절 번호
	char			Syllable[][3],	// 어절 (음절열)
	int			WordLen,		// 어절 길이 (음절의 수)
	double		PhoneticProb,	// 음운 복원 확률
	double		CutoffThreshold,
	int			BeamSize,
	MAtRESULTS		*Result		// [output]결과 저장
)
{
	PROBtFST *tag_s = Rsc->tag_s; // 전이 확률
	PROBtFST *syllable_s = Rsc->syllable_s; // 어휘 확률
	void *s_transition_fst = Rsc->s_transition_fst; // 음절 태그 전이 정보
	void *syllable_tag_fst = Rsc->syllable_tag_fst; // 음절-태그
	void *syllable_tag_info = Rsc->syllable_tag_info; // 음절-태그
		
	SVtSTRVCT *states;
	MAtRESULTS *results = NULL;  //  각 time별로 결과를 저장
	
	int i, j, k; // time, 현재, 이전
	int m;

	int start_time = 2; // 시작 위치
	int end_time = WordLen+1; // 끝 위치

	double transition_prob;
	double lexical_prob;
	
	char *prev_last_pos = NULL;
	
	static double global_max_prob;
	if (EjIndex == 0) global_max_prob = -LONG_MAX; // 어절 번호가 처음이면 초기화

	states = sv_NewArr( end_time+2); // 크기가 매우 중요함
	
	results = (MAtRESULTS *)malloc( sizeof( MAtRESULTS) * (end_time+2));
	
	// 초기화
	for (i = 0; i < WordLen+3; i++)
	{
		results[i].count = 0;
	}
	
	sv_Push( &states[0], BOW_TAG_2); // 어절 시작 태그
	sv_Push( &states[1], BOW_TAG_1); // 어절 시작 태그
	
	char init[100];
	
	sprintf( init, "\t%s\t%s", BOW_TAG_2, BOW_TAG_1);
	
#ifdef DEBUG
	fprintf(stderr, "음운복원 확률 = %lf\n", PhoneticProb);
#endif

	// 음운복원 확률
	results[1].mresult[0].str = strdup( init);
	results[1].mresult[0].prob = PhoneticProb;
	results[1].count++;
	

	///////////////////////////////////////////////////////////////////////////////
	// 초기화 (각 음절이 취할 수 있는 태그를 미리 넣어둔다.)
	{
		int j, n;
		int num;
		char *tag;

		for (i = start_time; i <= end_time; i++)
		{
#ifdef DEBUG
			fprintf( stderr, "[%d]%s", i, Syllable[i]);
#endif
			if ((n = fst_String2Hash( syllable_tag_fst, Syllable[i], &num)) == (-1)) // 리스트에 없으면
			{
#ifdef DEBUG
				// 이럴 땐 어쩐다???
				fprintf( stderr, "[%s] has no syllable tag\n", Syllable[i]);
#endif
			}
			else 
			{
				for (j = 0; j < num; j++) // 복수개의 분석 결과가 있을 경우
				{
					tag = b2t_GetString( syllable_tag_info, n++);
#ifdef DEBUG
					fprintf( stderr, " = %s\n", tag);
#endif
					// 어절 첫 태그가 'I' 태그가 될 수 없음 (속도 향상에 도움)
					if (i == start_time && tag[0] == 'I')
					{
#ifdef DEBUG
						fprintf(stderr, "I가 올 수 없음 (%s)\n", tag);
#endif
						continue;
					}
					
					sv_Push( &states[i], tag);
				}
			}
		}
		sv_Push( &states[end_time+1], EOW_TAG); // 어절 끝 태그
	}

	///////////////////////////////////////////////////////////////////////////////
	double cur_max_prob;
	double new_prob;
	
	char *cur_tag; // 현재 태그
	char *prev_tag; // 이전 태그

	// Iteration Step
	// end_time+1 -> 어절 끝 음절도 처리
	for (i = start_time; i <= end_time+1; i++)
	{
		cur_max_prob = -LONG_MAX; // 초기화

		// 현재 상태(태그)에 대해 // j
		for (j = 0; j < states[i].count; j++)
		{
			cur_tag = states[i].strs[j]; // 현재 태그

#ifdef DEBUG
			fprintf( stderr, "\ncur tag = %s\n", cur_tag);
#endif
			// 이전 상태(태그)에 대해 // k
			for (k = 0; k < states[i-1].count; k++)
			{
				prev_tag = states[i-1].strs[k]; // 이전 태그

#ifdef DEBUG
				fprintf( stderr, "\n\tprev tag = %s\n", prev_tag);
#endif
				// 현재 태그와 이전 태그와의 연결성 검사
				// for 속도 향상
				if ( !check_connectivity( cur_tag, prev_tag))
				{
#ifdef DEBUG
					fprintf( stderr, "stopped by conectivity constraint1 (%s->%s)\n", prev_tag, cur_tag);
#endif
					continue;
				}

				// 이전 품사와 현재 품사와의 연결성 검사
				{
					char two_tags[50];
					
					sprintf( two_tags, "%s%s", prev_tag, cur_tag);
					
					int num, n;
					if ((n = fst_String2Hash( s_transition_fst, two_tags, &num)) == (-1)) // 리스트에 없으면
					{
#ifdef DEBUG
						fprintf(stderr, "제약 걸림 %s -> %s\n", prev_tag, cur_tag);
#endif
						continue;
					}
				}

				// 연속해서 붙을 수 없는 태그 (현재, 이전)
				if ( !check_connectivity2( cur_tag, prev_tag))
				{
#ifdef DEBUG
					fprintf( stderr, "stopped by coonectivity constraint2 (%s->%s)\n", prev_tag, cur_tag);
#endif
					continue;
				}
				 
#ifdef DEBUG
				fprintf( stderr, "\ti = %d, 현재태그 = %s, 이전 태그 = %s\n", i, cur_tag, prev_tag);
#endif

				// 전이확률
				transition_prob = prob_GetFSTProb4( tag_s, 
										cur_tag,
										Syllable[i-1],
										prev_tag,
										Syllable[i]);
#ifdef DEBUG
				fprintf( stderr, "\ttransition prob = %lf\n", transition_prob);
#endif
				// 어휘 확률
				lexical_prob = prob_GetFSTProb3( syllable_s, 
										Syllable[i],
										Syllable[i-1],
										prev_tag);
#ifdef DEBUG
				fprintf(stderr, "\tlexical_prob = %lf\n", lexical_prob);
#endif
				// 이전 time의 모든 결과를 뒤진다.
				// 확률값의 내림차순으로 수행됨
				for (m = 0; m < results[i-1].count; m++)
				{
					if (m >= BeamSize) break; // 그만
					
					//**/fprintf( stderr, "%s로부터 ", results[i-1].mresult[m].str);
					prev_last_pos = strrchr( results[i-1].mresult[m].str, '\t');
					assert( prev_last_pos != NULL);
					prev_last_pos++;
					//**/fprintf( stderr, "찾은 마지막 태그 =  %s\n", prev_last_pos);
					
					// 이전 태그가 같지 않으면
					if ( strcmp( prev_last_pos, prev_tag) != 0)
					{
#ifdef DEBUG
						fprintf( stderr, "%s is not the last tag\n", prev_last_pos);
#endif
						continue;
					}

					// 이전까지의 확률 + 태그(전이)확률 + 음절(어휘)확률
					new_prob = results[i-1].mresult[m].prob + transition_prob + lexical_prob; 

					// cut-off가 가능한지 검사
					// 최대확률값과의 차가 threshold보다 크면 저장하지 않음
					if (CutoffThreshold > 0 && cur_max_prob - new_prob > CutoffThreshold)
					{
#ifdef DEBUG
						fprintf( stderr, "stopped by cutoff threshold\n");
#endif
						continue; // 여기서 중지
					}

					if (new_prob > cur_max_prob) 
					{
						cur_max_prob = new_prob; // 최대값보다 크면 최대값이 됨
					}
						
					if (CutoffThreshold > 0 && global_max_prob - new_prob > CutoffThreshold)
					{
#ifdef DEBUG
						fprintf( stderr, "stopped by global_max_prob1\n");
#endif
						continue;
					}
					
					char new_tag_seq[1024];
					
					sprintf( new_tag_seq, "%s\t%s", results[i-1].mresult[m].str, cur_tag);
					
					// 저장
					results[i].mresult[results[i].count].str = strdup( new_tag_seq); // 결과
					results[i].mresult[results[i].count].prob = new_prob; // 확률
					results[i].count++;

#ifdef DEBUG
					fprintf( stderr, "\t[saved %d/%d] %s %e\n", i, results[i].count-1, new_tag_seq, new_prob);
#endif

				} // for m
			} // for k
		} // for j
		
		// 정렬
		qsort( results[i].mresult, results[i].count, sizeof( MAtRESULT), ma_CompareProb);
	} // for i
	
	// 메모리 해제
	sv_FreeArr( states, WordLen+3);
	
	///////////////////////////////////////////////////////////////////////////////
	double sent_prob = 0.0; // 초기화 log(1)
	double cur_prob;
	cur_max_prob = -LONG_MAX;
	char *tag_seq;
	char *tags[1024];
	
	// 모든 결과에 대해
	for (j = 0; j < results[end_time+1].count; j++)
	{
		if (j >= BeamSize) // for 문 안에 넣어야 함
		{
#ifdef DEBUG
			fprintf(stderr, "beam size 초과\n");
#endif
			break;
		}
		
		cur_prob = results[end_time+1].mresult[j].prob;

		if (global_max_prob < cur_prob) global_max_prob = cur_prob;
		
		if (CutoffThreshold > 0 && global_max_prob - cur_prob > CutoffThreshold)
		{
#ifdef DEBUG
			fprintf( stderr, "stopped by global_max_prob2\n");
#endif
			break; // 여기서 중지
		}
		
		sent_prob = 0;
		
		tag_seq = strdup( results[end_time+1].mresult[j].str); // 태그열 복사
		
#ifdef DEBUG
		fprintf( stderr, "tag_seq = %s\n", tag_seq);
#endif

		getSyllableTags( tag_seq, tags); // 태그를 하나씩 얻어냄
		
		// 전체 확률을 3-gram 모델을 이용하여 다시 구한다.
		// 모든 음절/태그에 대해
		for (i = start_time; i <= end_time+1; i++) // 등호에 주의!
		{
			transition_prob = prob_GetFSTProb5( tag_s, 
									tags[i],
									tags[i-2],
									Syllable[i-1],
									tags[i-1],
									Syllable[i]);

#ifdef DEBUG
			fprintf( stderr, "##transition prob = %lf\n", transition_prob);
#endif

			lexical_prob = prob_GetFSTProb5( syllable_s,
									Syllable[i],
									Syllable[i-2],
									tags[i-2],
									Syllable[i-1],
									tags[i-1]);
#ifdef DEBUG
			fprintf( stderr, "##lexical prob = %lf\n", lexical_prob);
#endif
					
			sent_prob += transition_prob + lexical_prob;
		}
		
		free( tag_seq);
		
		// 음운 복원 확률
		sent_prob += PhoneticProb;
			
		if (sent_prob > cur_max_prob) cur_max_prob = sent_prob;
			
		if (CutoffThreshold > 0 && cur_max_prob - sent_prob > CutoffThreshold)
		{
#ifdef DEBUG
			fprintf( stderr, "stopped\n");
#endif
		}
		else 
		{
			// 결과 저장
			Result->mresult[Result->count].str = strdup( results[end_time+1].mresult[j].str );
			Result->mresult[Result->count].prob = sent_prob;
			Result->count++;
#ifdef DEBUG
			fprintf( stderr, "saved in Result[%d]\n", Result->count-1);
#endif
		}
			
#ifdef DEBUG
		fprintf(stderr, "\n");
#endif
	} // for j

	// 메모리 해제
	ma_FreeAll( results, end_time+2);
	
}
