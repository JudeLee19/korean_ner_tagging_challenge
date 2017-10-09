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

#define KMATcMAX_STATE_NUM	50	// ����(state)�� ���� �ִ밪

// Ÿ�� ����
typedef struct 
{
	double	lexical_prob;		// (���� ���¿� ����) ���¼� �м� Ȯ��
	double	path_prob;			// (���� ���±����� ��ο� ����) Ȯ��
	int		prev_state;			// (�ִ� Ȯ�� ��θ� ����) ���� ����
	char		first_tag[8];		// ù �±�
	char		second_tag[8];		// ��° �±�
	char		last_tag[8];		// ������ �±�
	char		second_last_tag[8];	// ���������� �ϳ� ���� �±�
} tBIGRAM_NODE;

typedef struct {
  int			count;
  tBIGRAM_NODE	node[KMATcMAX_STATE_NUM];
} tBIGRAM_STATES; // Ư�� time�� ���� ����

///////////////////////////////////////////////////////////////////////////////
void ViterbiSearch2gram
(
	PROBtFST		*IntraTrans,		// [input]
	PROBtFST		*InterTrans,		// [input]
	MAtRESULTS		*MorphAnalyzedResult,	// [input] ���¼� �м� ��� ��
	int			NumWord,			// [input] ���� ��
	int			*StateSeq			// [output] ����(���¼� �м� ����� ��ȣ)��(��� ����)
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

	int morph_num = 0; // ���� ���� ���¼� ��
	SVtSTRVCT *morphs;
	SVtSTRVCT *tags;
	
	// �ʱ�ȭ
	states[0].node[0].path_prob = 0.0; // log (1)
	strcpy( states[0].node[0].last_tag, BOSTAG_1);
	
	// trigram
	strcpy( states[0].node[0].second_last_tag, BOSTAG_2);
	states[0].count = 1;

	for (i = 1; i <= NumWord; i++) // ��� ������ ����
	{
		
		states[i].count = ((MorphAnalyzedResult[i].count > KMATcMAX_STATE_NUM) ? KMATcMAX_STATE_NUM : MorphAnalyzedResult[i].count);
		
		for (j = 0; j < states[i].count; j++) // ��� �м� ����� ���� (��, ���¼� �м� ����� ���� KMATcMAX_STATE_NUM ���� �۾ƾ� ��)
		{
			states[i].node[j].path_prob = -LONG_MAX;
			states[i].node[j].lexical_prob = MorphAnalyzedResult[i].mresult[j].prob; // ���¼Һм� Ȯ��
#ifdef DEBUG
			fprintf( stdout, "ma_prob : [%d]%lf\n", j, MorphAnalyzedResult[i].mresult[j].prob);
#endif
		}

		// �ι�°���͸� �ǹ̰� ���� (���Ǽ��� �����Ƿ�)
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

			strcpy( states[i].node[j].first_tag, tags->strs[0]); // ù��° �±�
			strcpy( states[i].node[j].last_tag, tags->strs[morph_num-1]); // ������ �±�

			if (morph_num > 1) 
			{
				strcpy( states[i].node[j].second_tag, tags->strs[1]); // �ι�° �±�
				strcpy( states[i].node[j].second_last_tag, tags->strs[morph_num-2]); // ������-1 �±�
			}
			else // �ϳ��� ���¼Ҹ� ���� �����̸�
			{
				strcpy( states[i].node[j].second_tag, EOW_TAG);
				strcpy( states[i].node[j].second_last_tag, BOW_TAG_1);
			}

			///**/fprintf(stderr, "first tag = %s, last tag = %s\n", states[i].node[j].first_tag, states[i].node[j].last_tag);

			// ������ ���� ���⼭ lexical_prob�� ���Ѵ�.
				
			// �̵��� ���� ���⼭ ù��° ǰ��� ������ ǰ���� ���� Ȯ��(�и� �ش��ϴ�)�� ���Ѵ�.
			// ���¼Һм� Ȯ��(lexcial_prob)���� ���� (�ֳĸ�, �и�ϱ�)

#ifdef USING_DENOMINATOR

#ifdef TRIGRAM_TAGGING
			// trigram �� ////////////////////////////
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
	// �� time(token) ����
	for (i = 1; i <= NumWord; i++)
	{
		// ���� ����(�±�)�� ����
		for (j = 0; j < states[i].count; j++)
		{
			max_path_prob = -LONG_MAX;			 // log(0.0)// �ʱ�ȭ
			max_prev_state = 0; // must be here...

			// ���� ����(�±�)�� ����
			for (k = 0; k < states[i-1].count; k++)
			{ 
#ifdef DEBUG 
				fprintf (stdout, "-----------------------------\n");
				fprintf (stdout, "get_probability([%d]%s, prev_tag = %s, cur_tag = %s)\n",
							i, MorphAnalyzedResult[i].mresult[j].str,
							states[i-1].node[k].last_tag, states[i].node[j].first_tag);
#endif
				
#ifdef TRIGRAM_TAGGING
				// trigram �� ////////////////////////////
				{
					double transition_prob1 = 0.0;
					double transition_prob2 = 0.0;
					

					sprintf( denom, "%s%c%s->", states[i-1].node[k].second_last_tag, PROBcDELIM_INTRA, states[i-1].node[k].last_tag);
					
					//fprintf( stderr, "denom = %s\n", denom);
	 
					transition_prob1 = prob_GetFSTProb2( InterTrans, states[i].node[j].first_tag, denom); // ������ ����Ȯ��
#ifdef DEBUG
					fprintf( stdout, "inter_trans_prob : p(%s|%s) = %lf\n", states[i].node[j].first_tag, denom, transition_prob1);
#endif
					sprintf( denom, "%s->%s", states[i-1].node[k].last_tag, states[i].node[j].first_tag);

					transition_prob2 = prob_GetFSTProb2( InterTrans, states[i].node[j].second_tag, denom); // ������ ����Ȯ��
#ifdef DEBUG
					fprintf( stdout, "inter_trans_prob : p(%s|%s) = %lf\n", states[i].node[j].second_tag, denom, transition_prob2);
#endif

					// Ȯ�� ���
					cur_path_prob = states[i-1].node[k].path_prob //prevt_itr->second.path_prob // ���� ���±����� ��ο� ���� Ȯ�� // SeqPrb[i - 1][k]
							+ transition_prob1 + transition_prob2
							+ states[i].node[j].lexical_prob; //curt_itr->second.lexical_prob; // ���� Ȯ��

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
				// bigram �� ////////////////////////////
				{
					// Ȯ�� ���
					cur_path_prob = states[i-1].node[k].path_prob // ���� ���±����� ��ο� ���� Ȯ�� // SeqPrb[i - 1][k]
							+ map_get_probability2( inter_transition_prob, states[i].node[j].first_tag, states[i-1].node[k].last_tag) // ������ ����Ȯ��
							+ states[i].node[j].lexical_prob; // ���� Ȯ��
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
					max_prev_state = k; // �Ҿ��ϴ�. prevt_itr->first;
				}
			} // for (k = 0; k < states[i-1].count; k++)

			states[i].node[j].path_prob = max_path_prob; // arg ; �ִ� Ȯ��
			states[i].node[j].prev_state = max_prev_state; // argmax ; ���� ����

#ifdef DEBUG
			fprintf ( stdout, "max prob = %lf, max prev state = %d\n",
					states[i].node[j].path_prob, states[i].node[j].prev_state);
#endif
			
		} // for (j = 0; j < states[i].count; j++)
	} // for (i = 1; i <= NumWord; i++)
	
	// Termination and path-readout
	max_path_prob = -LONG_MAX;	 // �ʱ�ȭ

	for (j = 0; j < states[NumWord].count; j++)
	{
		cur_path_prob = states[NumWord].node[j].path_prob;

		if (max_path_prob < cur_path_prob) 
		{
			max_path_prob = cur_path_prob;
			max_prev_state = j; // curt_itr->first; // �Ҿ��ϴ�.
		}
	}
	
	// ��� ����
	// state_sequence�� 1���� total_time���� �����
	StateSeq[NumWord] = max_prev_state;

#ifdef DEBUG
	fprintf ( stdout, "StateSeq[NumWord] = %d\n", StateSeq[NumWord]);
#endif
	
	for (i = NumWord - 1; i >= 1; i--) 
	{
		StateSeq[i] = states[i+1].node[StateSeq[i+1]].prev_state;
	}

#ifdef DEBUG 
	fprintf( stdout, "ǰ�� �±׿�\n");
	for (i = 1; i <= NumWord; i++) 
	{
		fprintf( stdout, "[%d]", StateSeq[i]);
	}
	fprintf( stdout, " %.3f\n", max_path_prob);
	fflush( stdout);
#endif
}
