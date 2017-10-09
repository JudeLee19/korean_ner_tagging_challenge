//#define DEBUG

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>               // LONG_MAX
#include <assert.h>

#include "kmat_def.h"
#include "kmat.h"
#include "ma.h"
#include "probtool.h"
#include "hsplit.h"
#include "get_morph_tag.h"
#include "strvector.h"

#define KMATcMAX_STATE_NUM	50	// 상태(state)의 수의 최대값

// 타입 선언
typedef struct 
{
	double	lexical_prob;		// (현재 상태에 대한) 형태소 분석 확률
	double	path_prob;			// (현재 상태까지의 경로에 대한) 확률
	int		prev_state;			// (최대 확률 경로를 가진) 이전 상태
	char		first_tag[8];		// 첫 태그
	char		second_tag[8];		// 둘째 태그
	char		last_tag[8];		// 마지막 태그
	char		second_last_tag[8];	// 마지막에서 하나 이전 태그
} tBIGRAM_NODE;

typedef struct {
  int			count;
  tBIGRAM_NODE	node[KMATcMAX_STATE_NUM];
} tBIGRAM_STATES; // 특정 time의 상태 노드들

///////////////////////////////////////////////////////////////////////////////
void ViterbiSearch2gram
(
	PROBtFST		*IntraTrans,		// [input]
	PROBtFST		*InterTrans,		// [input]
	MAtRESULTS		*MorphAnalyzedResult,	// [input] 형태소 분석 결과 열
	int			NumWord,			// [input] 어절 수
	int			*StateSeq			// [output] 상태(형태소 분석 결과의 번호)열(결과 저장)
)
{
	int i, j, k;

#ifdef WIN32
	tBIGRAM_STATES states[500];
#else
	tBIGRAM_STATES states[NumWord+1];
#endif

	int max_prev_state = 0;
	double max_path_prob;
	double cur_path_prob;

	int morph_num = 0; // 어절 내의 형태소 수
	SVtSTRVCT *morphs;
	SVtSTRVCT *tags;
	
	// 초기화
	states[0].node[0].path_prob = 0.0; // log (1)
	strcpy( states[0].node[0].last_tag, BOSTAG_1);
	
	// trigram
	strcpy( states[0].node[0].second_last_tag, BOSTAG_2);
	states[0].count = 1;

	for (i = 1; i <= NumWord; i++) // 모든 어절에 대해
	{
		
		states[i].count = ((MorphAnalyzedResult[i].count > KMATcMAX_STATE_NUM) ? KMATcMAX_STATE_NUM : MorphAnalyzedResult[i].count);
		
		for (j = 0; j < states[i].count; j++) // 모든 분석 결과에 대해 (단, 형태소 분석 결과의 수가 KMATcMAX_STATE_NUM 보다 작아야 함)
		{
			states[i].node[j].path_prob = -LONG_MAX;
			states[i].node[j].lexical_prob = MorphAnalyzedResult[i].mresult[j].prob; // 형태소분석 확률
#ifdef DEBUG
			fprintf( stdout, "ma_prob : [%d]%lf\n", j, MorphAnalyzedResult[i].mresult[j].prob);
#endif
		}

		// 두번째부터만 의미가 있음 (중의성이 있으므로)
		for (j = 0; j < states[i].count; j++) 
		{
#ifdef DEBUG
			fprintf( stdout, "[%d][%d]%s\n", i, j, MorphAnalyzedResult[i].mresult[j].str);
#endif
			morphs = sv_New();
			tags = sv_New();
			
			morph_num = GetMorphTag( MorphAnalyzedResult[i].mresult[j].str, DELIMITER, morphs, tags);
			
#ifdef DEBUG
			fprintf( stdout, "# of morphemes = %d\n", morph_num);
#endif

			if (morph_num < 1)
			{
				fprintf(stderr, "number of morpheme is 0\n"); 
				exit(1);
			}

			strcpy( states[i].node[j].first_tag, tags->strs[0]); // 첫번째 태그
			strcpy( states[i].node[j].last_tag, tags->strs[morph_num-1]); // 마지막 태그

			if (morph_num > 1) 
			{
				strcpy( states[i].node[j].second_tag, tags->strs[1]); // 두번째 태그
				strcpy( states[i].node[j].second_last_tag, tags->strs[morph_num-2]); // 마지막-1 태그
			}
			else // 하나의 형태소를 가진 어절이면
			{
				strcpy( states[i].node[j].second_tag, EOW_TAG);
				strcpy( states[i].node[j].second_last_tag, BOW_TAG_1);
			}

			///**/fprintf(stderr, "first tag = %s, last tag = %s\n", states[i].node[j].first_tag, states[i].node[j].last_tag);

			// 김진동 모델은 여기서 lexical_prob을 구한다.
				
			// 이도길 모델은 여기서 첫번째 품사와 마지막 품사의 전이 확률(분모에 해당하는)을 구한다.
			// 형태소분석 확률(lexcial_prob)에서 빼자 (왜냐면, 분모니까)

#ifdef USING_DENOMINATOR

#ifdef TRIGRAM_TAGGING
			// trigram 모델 ////////////////////////////
			{
				char denom[20];
				sprintf( denom, "%s%c%s", BOW_TAG_2, PROBcDELIM_INTRA, BOW_TAG_1);
				states[i].node[j].lexical_prob -= prob_GetFSTProb2( IntraTrans, states[i].node[j].first_tag, denom);

#ifdef DEBUG
				fprintf( stdout, "intra_trans_prob : p(%s|%s) = %lf\n", states[i].node[j].first_tag, denom, prob_GetFSTProb2( IntraTrans, states[i].node[j].first_tag, denom));
#endif
				//denom = BOW_TAG_1;
				//denom += tags->strs[0];
				sprintf( denom, "%s%c%s", BOW_TAG_1, PROBcDELIM_INTRA, tags->strs[0]);
				states[i].node[j].lexical_prob -= prob_GetFSTProb2( IntraTrans, states[i].node[j].second_tag, denom); // <-
#ifdef DEBUG
				fprintf( stdout, "intra_trans_prob : p(%s|%s) = %lf\n", states[i].node[j].second_tag, denom, prob_GetFSTProb2( IntraTrans, states[i].node[j].second_tag, denom));
#endif
			}
#endif
#endif // USING_DENOMINATOR

			sv_Free( morphs);
			sv_Free( tags);

		}	// end of for
	} // end of for
	
	char denom[1000];

	// Iteration Step
	// 각 time(token) 마다
	for (i = 1; i <= NumWord; i++)
	{
		// 현재 상태(태그)에 대해
		for (j = 0; j < states[i].count; j++)
		{
			max_path_prob = -LONG_MAX;			 // log(0.0)// 초기화
			max_prev_state = 0; // must be here...

			// 이전 상태(태그)에 대해
			for (k = 0; k < states[i-1].count; k++)
			{ 
#ifdef DEBUG 
				fprintf (stdout, "-----------------------------\n");
				fprintf (stdout, "get_probability([%d]%s, prev_tag = %s, cur_tag = %s)\n",
							i, MorphAnalyzedResult[i].mresult[j].str,
							states[i-1].node[k].last_tag, states[i].node[j].first_tag);
#endif
				
#ifdef TRIGRAM_TAGGING
				// trigram 모델 ////////////////////////////
				{
					double transition_prob1 = 0.0;
					double transition_prob2 = 0.0;
					

					sprintf( denom, "%s%c%s->", states[i-1].node[k].second_last_tag, PROBcDELIM_INTRA, states[i-1].node[k].last_tag);
					
					//fprintf( stderr, "denom = %s\n", denom);
	 
					transition_prob1 = prob_GetFSTProb2( InterTrans, states[i].node[j].first_tag, denom); // 어절간 전이확률
#ifdef DEBUG
					fprintf( stdout, "inter_trans_prob : p(%s|%s) = %lf\n", states[i].node[j].first_tag, denom, transition_prob1);
#endif
					sprintf( denom, "%s->%s", states[i-1].node[k].last_tag, states[i].node[j].first_tag);

					transition_prob2 = prob_GetFSTProb2( InterTrans, states[i].node[j].second_tag, denom); // 어절간 전이확률
#ifdef DEBUG
					fprintf( stdout, "inter_trans_prob : p(%s|%s) = %lf\n", states[i].node[j].second_tag, denom, transition_prob2);
#endif

					// 확률 계산
					cur_path_prob = states[i-1].node[k].path_prob //prevt_itr->second.path_prob // 이전 상태까지의 경로에 대한 확률 // SeqPrb[i - 1][k]
							+ transition_prob1 + transition_prob2
							+ states[i].node[j].lexical_prob; //curt_itr->second.lexical_prob; // 어휘 확률

#ifdef DEBUG
					fprintf ( stdout, "prev sequence + transition + lexical = %lf + %lf + %lf = %lf\n", 
						states[i-1].node[k].path_prob,
						transition_prob1 + transition_prob2,
						states[i].node[j].lexical_prob, 
						cur_path_prob);
#endif
				}
				
#endif

#ifdef BIGRAM_TAGGING
				// bigram 모델 ////////////////////////////
				{
					// 확률 계산
					cur_path_prob = states[i-1].node[k].path_prob // 이전 상태까지의 경로에 대한 확률 // SeqPrb[i - 1][k]
							+ map_get_probability2( inter_transition_prob, states[i].node[j].first_tag, states[i-1].node[k].last_tag) // 어절간 전이확률
							+ states[i].node[j].lexical_prob; // 어휘 확률
				}
#ifdef DEBUG
				fprintf ( stdout, "prev sequence + transition + lexical = %lf + %lf + %lf = %lf\n", 
						states[i-1].node[k].path_prob, 
						map_get_probability2( inter_transition_prob, states[i].node[j].first_tag, states[i-1].node[k].last_tag), 
						states[i].node[j].lexical_prob, 
						cur_path_prob);
#endif
#endif

				if (max_path_prob < cur_path_prob) 
				{
					max_path_prob = cur_path_prob;
					max_prev_state = k; // 불안하다. prevt_itr->first;
				}
			} // for (k = 0; k < states[i-1].count; k++)

			states[i].node[j].path_prob = max_path_prob; // arg ; 최대 확률
			states[i].node[j].prev_state = max_prev_state; // argmax ; 이전 상태

#ifdef DEBUG
			fprintf ( stdout, "max prob = %lf, max prev state = %d\n",
					states[i].node[j].path_prob, states[i].node[j].prev_state);
#endif
			
		} // for (j = 0; j < states[i].count; j++)
	} // for (i = 1; i <= NumWord; i++)
	
	// Termination and path-readout
	max_path_prob = -LONG_MAX;	 // 초기화

	for (j = 0; j < states[NumWord].count; j++)
	{
		cur_path_prob = states[NumWord].node[j].path_prob;

		if (max_path_prob < cur_path_prob) 
		{
			max_path_prob = cur_path_prob;
			max_prev_state = j; // curt_itr->first; // 불안하다.
		}
	}
	
	// 경로 저장
	// state_sequence에 1부터 total_time까지 저장됨
	StateSeq[NumWord] = max_prev_state;

#ifdef DEBUG
	fprintf ( stdout, "StateSeq[NumWord] = %d\n", StateSeq[NumWord]);
#endif
	
	for (i = NumWord - 1; i >= 1; i--) 
	{
		StateSeq[i] = states[i+1].node[StateSeq[i+1]].prev_state;
	}

#ifdef DEBUG 
	fprintf( stdout, "품사 태그열\n");
	for (i = 1; i <= NumWord; i++) 
	{
		fprintf( stdout, "[%d]", StateSeq[i]);
	}
	fprintf( stdout, " %.3f\n", max_path_prob);
	fflush( stdout);
#endif
}
