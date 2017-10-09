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
// ���� �±׿� ���� �±׿��� ���Ἲ �˻�
// ���ϰ� : 1 = ���� ����, 0 = ���� �Ұ���
static int check_connectivity
(
	const char	*CurTag,
	const char	*PrevTag
)
{
	// B_STYLE �϶�
	if (CurTag[0] == 'I') 
	{
		if (strcmp( &CurTag[2], &PrevTag[2]) != 0) 
			return 0;
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// ���� �±׿� ���� �±׿��� ���Ἲ �˻�
// ���ϰ� : 1 = ���� ����, 0 = ���� �Ұ���
static int check_connectivity2
(
	const char	*CurTag,
	const char	*PrevTag
)
{
	 // ��) ���� �±װ� SL�϶�(�����̰ų� �߰��̰ų� �������) ���� �±װ� SL�� �����̸� ���� �Ұ�

	 if (strcmp( &PrevTag[2], "SL") == 0 && strcmp( CurTag, "B-SL") == 0) // �ܱ���
		 return 0;

	 if (strcmp( &PrevTag[2], "SH") == 0 && strcmp( CurTag, "B-SH") == 0) // ����
		 return 0;

	 if (strcmp( &PrevTag[2], "SN") == 0 && strcmp( CurTag, "B-SN") == 0) // ����
		 return 0;

	 if (strcmp( &PrevTag[2], "SE") == 0 && strcmp( CurTag, "B-SE") == 0) // ����ǥ
		 return 0;

	 if (strcmp( &PrevTag[2], "SO") == 0 && strcmp( CurTag, "B-SO") == 0) // ����ǥ
			 return 0;

	 return 1;
}

///////////////////////////////////////////////////////////////////////////////
void bigram_BFS
(
	MAtRSC_S		*Rsc,			// ���ҽ�
	int			EjIndex,		// ���� ��ȣ
	char			Syllable[][3],	// ���� (������)
	int			WordLen,		// ���� ���� (������ ��)
	double		PhoneticProb,	// ���� ���� Ȯ��
	double		CutoffThreshold,
	int			BeamSize,
	MAtRESULTS		*Result		// [output]��� ����
)
{
	PROBtFST *tag_s = Rsc->tag_s; // ���� Ȯ��
	PROBtFST *syllable_s = Rsc->syllable_s; // ���� Ȯ��
	void *s_transition_fst = Rsc->s_transition_fst; // ���� �±� ���� ����
	void *syllable_tag_fst = Rsc->syllable_tag_fst; // ����-�±�
	void *syllable_tag_info = Rsc->syllable_tag_info; // ����-�±�
		
	SVtSTRVCT *states;
	MAtRESULTS *results = NULL;  //  �� time���� ����� ����
	
	int i, j, k; // time, ����, ����
	int m;

	int start_time = 2; // ���� ��ġ
	int end_time = WordLen+1; // �� ��ġ

	double transition_prob;
	double lexical_prob;
	
	char *prev_last_pos = NULL;
	
	static double global_max_prob;
	if (EjIndex == 0) global_max_prob = -LONG_MAX; // ���� ��ȣ�� ó���̸� �ʱ�ȭ

	states = sv_NewArr( end_time+2); // ũ�Ⱑ �ſ� �߿���
	
	results = (MAtRESULTS *)malloc( sizeof( MAtRESULTS) * (end_time+2));
	
	// �ʱ�ȭ
	for (i = 0; i < WordLen+3; i++)
	{
		results[i].count = 0;
	}
	
	sv_Push( &states[0], BOW_TAG_2); // ���� ���� �±�
	sv_Push( &states[1], BOW_TAG_1); // ���� ���� �±�
	
	char init[100];
	
	sprintf( init, "\t%s\t%s", BOW_TAG_2, BOW_TAG_1);
	
#ifdef DEBUG
	fprintf(stderr, "����� Ȯ�� = %lf\n", PhoneticProb);
#endif

	// ����� Ȯ��
	results[1].mresult[0].str = strdup( init);
	results[1].mresult[0].prob = PhoneticProb;
	results[1].count++;
	

	///////////////////////////////////////////////////////////////////////////////
	// �ʱ�ȭ (�� ������ ���� �� �ִ� �±׸� �̸� �־�д�.)
	{
		int j, n;
		int num;
		char *tag;

		for (i = start_time; i <= end_time; i++)
		{
#ifdef DEBUG
			fprintf( stderr, "[%d]%s", i, Syllable[i]);
#endif
			if ((n = fst_String2Hash( syllable_tag_fst, Syllable[i], &num)) == (-1)) // ����Ʈ�� ������
			{
#ifdef DEBUG
				// �̷� �� ��¾��???
				fprintf( stderr, "[%s] has no syllable tag\n", Syllable[i]);
#endif
			}
			else 
			{
				for (j = 0; j < num; j++) // �������� �м� ����� ���� ���
				{
					tag = b2t_GetString( syllable_tag_info, n++);
#ifdef DEBUG
					fprintf( stderr, " = %s\n", tag);
#endif
					// ���� ù �±װ� 'I' �±װ� �� �� ���� (�ӵ� ��� ����)
					if (i == start_time && tag[0] == 'I')
					{
#ifdef DEBUG
						fprintf(stderr, "I�� �� �� ���� (%s)\n", tag);
#endif
						continue;
					}
					
					sv_Push( &states[i], tag);
				}
			}
		}
		sv_Push( &states[end_time+1], EOW_TAG); // ���� �� �±�
	}

	///////////////////////////////////////////////////////////////////////////////
	double cur_max_prob;
	double new_prob;
	
	char *cur_tag; // ���� �±�
	char *prev_tag; // ���� �±�

	// Iteration Step
	// end_time+1 -> ���� �� ������ ó��
	for (i = start_time; i <= end_time+1; i++)
	{
		cur_max_prob = -LONG_MAX; // �ʱ�ȭ

		// ���� ����(�±�)�� ���� // j
		for (j = 0; j < states[i].count; j++)
		{
			cur_tag = states[i].strs[j]; // ���� �±�

#ifdef DEBUG
			fprintf( stderr, "\ncur tag = %s\n", cur_tag);
#endif
			// ���� ����(�±�)�� ���� // k
			for (k = 0; k < states[i-1].count; k++)
			{
				prev_tag = states[i-1].strs[k]; // ���� �±�

#ifdef DEBUG
				fprintf( stderr, "\n\tprev tag = %s\n", prev_tag);
#endif
				// ���� �±׿� ���� �±׿��� ���Ἲ �˻�
				// for �ӵ� ���
				if ( !check_connectivity( cur_tag, prev_tag))
				{
#ifdef DEBUG
					fprintf( stderr, "stopped by conectivity constraint1 (%s->%s)\n", prev_tag, cur_tag);
#endif
					continue;
				}

				// ���� ǰ��� ���� ǰ����� ���Ἲ �˻�
				{
					char two_tags[50];
					
					sprintf( two_tags, "%s%s", prev_tag, cur_tag);
					
					int num, n;
					if ((n = fst_String2Hash( s_transition_fst, two_tags, &num)) == (-1)) // ����Ʈ�� ������
					{
#ifdef DEBUG
						fprintf(stderr, "���� �ɸ� %s -> %s\n", prev_tag, cur_tag);
#endif
						continue;
					}
				}

				// �����ؼ� ���� �� ���� �±� (����, ����)
				if ( !check_connectivity2( cur_tag, prev_tag))
				{
#ifdef DEBUG
					fprintf( stderr, "stopped by coonectivity constraint2 (%s->%s)\n", prev_tag, cur_tag);
#endif
					continue;
				}
				 
#ifdef DEBUG
				fprintf( stderr, "\ti = %d, �����±� = %s, ���� �±� = %s\n", i, cur_tag, prev_tag);
#endif

				// ����Ȯ��
				transition_prob = prob_GetFSTProb4( tag_s, 
										cur_tag,
										Syllable[i-1],
										prev_tag,
										Syllable[i]);
#ifdef DEBUG
				fprintf( stderr, "\ttransition prob = %lf\n", transition_prob);
#endif
				// ���� Ȯ��
				lexical_prob = prob_GetFSTProb3( syllable_s, 
										Syllable[i],
										Syllable[i-1],
										prev_tag);
#ifdef DEBUG
				fprintf(stderr, "\tlexical_prob = %lf\n", lexical_prob);
#endif
				// ���� time�� ��� ����� ������.
				// Ȯ������ ������������ �����
				for (m = 0; m < results[i-1].count; m++)
				{
					if (m >= BeamSize) break; // �׸�
					
					//**/fprintf( stderr, "%s�κ��� ", results[i-1].mresult[m].str);
					prev_last_pos = strrchr( results[i-1].mresult[m].str, '\t');
					assert( prev_last_pos != NULL);
					prev_last_pos++;
					//**/fprintf( stderr, "ã�� ������ �±� =  %s\n", prev_last_pos);
					
					// ���� �±װ� ���� ������
					if ( strcmp( prev_last_pos, prev_tag) != 0)
					{
#ifdef DEBUG
						fprintf( stderr, "%s is not the last tag\n", prev_last_pos);
#endif
						continue;
					}

					// ���������� Ȯ�� + �±�(����)Ȯ�� + ����(����)Ȯ��
					new_prob = results[i-1].mresult[m].prob + transition_prob + lexical_prob; 

					// cut-off�� �������� �˻�
					// �ִ�Ȯ�������� ���� threshold���� ũ�� �������� ����
					if (CutoffThreshold > 0 && cur_max_prob - new_prob > CutoffThreshold)
					{
#ifdef DEBUG
						fprintf( stderr, "stopped by cutoff threshold\n");
#endif
						continue; // ���⼭ ����
					}

					if (new_prob > cur_max_prob) 
					{
						cur_max_prob = new_prob; // �ִ밪���� ũ�� �ִ밪�� ��
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
					
					// ����
					results[i].mresult[results[i].count].str = strdup( new_tag_seq); // ���
					results[i].mresult[results[i].count].prob = new_prob; // Ȯ��
					results[i].count++;

#ifdef DEBUG
					fprintf( stderr, "\t[saved %d/%d] %s %e\n", i, results[i].count-1, new_tag_seq, new_prob);
#endif

				} // for m
			} // for k
		} // for j
		
		// ����
		qsort( results[i].mresult, results[i].count, sizeof( MAtRESULT), ma_CompareProb);
	} // for i
	
	// �޸� ����
	sv_FreeArr( states, WordLen+3);
	
	///////////////////////////////////////////////////////////////////////////////
	double sent_prob = 0.0; // �ʱ�ȭ log(1)
	double cur_prob;
	cur_max_prob = -LONG_MAX;
	char *tag_seq;
	char *tags[1024];
	
	// ��� ����� ����
	for (j = 0; j < results[end_time+1].count; j++)
	{
		if (j >= BeamSize) // for �� �ȿ� �־�� ��
		{
#ifdef DEBUG
			fprintf(stderr, "beam size �ʰ�\n");
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
			break; // ���⼭ ����
		}
		
		sent_prob = 0;
		
		tag_seq = strdup( results[end_time+1].mresult[j].str); // �±׿� ����
		
#ifdef DEBUG
		fprintf( stderr, "tag_seq = %s\n", tag_seq);
#endif

		getSyllableTags( tag_seq, tags); // �±׸� �ϳ��� ��
		
		// ��ü Ȯ���� 3-gram ���� �̿��Ͽ� �ٽ� ���Ѵ�.
		// ��� ����/�±׿� ����
		for (i = start_time; i <= end_time+1; i++) // ��ȣ�� ����!
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
		
		// ���� ���� Ȯ��
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
			// ��� ����
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

	// �޸� ����
	ma_FreeAll( results, end_time+2);
	
}
