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
// ���ҽ� ����
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
// result_s�� ����� ����� �����Ͽ� analyzed_result_s�� ����
// ���ϰ� : �м� ����� ��
static int arrange_result_s
(
	MAtRESULTS	*Result_in,
	MAtRESULTS	*Result_out, // [output]
	double	CutoffThreshold
)
{

	char result_str[MAX_WORD]; // FIL�� ������ ���
	int i;
	
	Result_out->count = 0; // �ʱ�ȭ
	
	if (Result_in->count < 1) return 0; // ����� ������ ����

	double max_prob = Result_in->mresult[0].prob; // �ְ� Ȯ����

	// �ݺ�
	for (i = 0; i < Result_in->count; i++)
	{
		if (CutoffThreshold > 0)	// �� ���� 0 �̻��� ��쿡�� �����
		{
			// cut-off�� �������� �˻�
			// �ְ� Ȯ���� ���� ������� �αװ��� ���� ����ġ�̻��̸� ����
			if (max_prob - Result_in->mresult[i].prob > CutoffThreshold) 
			{
				// ���⼭���� ������������ ������ �ʿ���� (������ ����� Ȯ������ �� �����Ƿ�)
				break;
			}
		}
		
		if (strstr( Result_in->mresult[i].str, "/NA") != NULL) continue; // �м� ����� NA�� ������ �ȵ�
		
		// ������� �� ���� �����ؾ� ��
		convert_str_origin_array( Result_in->mresult[i].str, result_str); // ������� FIL�� ����
		Result_out->mresult[Result_out->count].str = strdup( result_str );
		Result_out->mresult[Result_out->count].prob = Result_in->mresult[i].prob; // �α� Ȯ��
		Result_out->count++;
	}

	return Result_out->count; // �м��� ����� ��
}

///////////////////////////////////////////////////////////////////////////////
// Ȯ���� ���¼� �м� (���� ����)
int ma_ExecS
(
	MAtRSC_S	*Rsc,
	MAtRESULTS	*RestoredEJ,	// ���� ������ ����
	MAtRESULTS	*Result,		// [output]
	double	CutoffThreshold,
	int		BeamSize
)
{
	char rest_ej[MAX_WORD]; // FIL ���ŵ� (�������) ����
	char splitchar[MAX_WORD][3]; // �������� �и��� ������ ���ڸ� ��� 2����Ʈ�� ��ȯ�Ͽ� ����
	int num_splitchar = 0;

	char morph_result[MAX_WORD]; // ��� ����
	int i, j;

	MAtRESULTS *result_s;

	result_s = (MAtRESULTS *) malloc( sizeof (MAtRESULTS));
	assert( result_s != NULL);
	
	result_s->count = 0;	

	double max_restored_prob = RestoredEJ->mresult[0].prob; // ù ���� ���� Ȯ��
	
	int ej_num = 0; // ������ ������ ��ȣ

#ifdef DEBUG	
	fprintf( stderr, "������ ������ �� = %d (line %d in %s)\n", RestoredEJ->count, __LINE__, __FILE__);
	fprintf( stderr, "CutoffThreshold = %f\n", CutoffThreshold);
#endif
	// ��� ������ ������ ���� �ݺ�
	for (i = 0; i < RestoredEJ->count; i++, ej_num++)
	{
		convert_str_origin_array( RestoredEJ->mresult[i].str, rest_ej); // FIL ����
		num_splitchar = split_by_char( rest_ej, &splitchar[2]);

		// �� ���� ó��
/*		if (num_splitchar >= 40) {
			fprintf(stderr, "Error: too long word! [%s]\n", rest_ej);
			continue;
		}
*/
		// ���� ���� 15���� ���� ������ ù��° ������ ������ ���ؼ��� ����
		if (i && num_splitchar >= 15) 
		{
#ifdef DEBUG	
			fprintf( stderr, "������ ������ ���� �ʰ� (%d) (line %d in %s)\n", num_splitchar, __LINE__, __FILE__);
#endif
			break;
		}


		// ���� ���� Ȯ������ �ʹ� ���� ��쳪 0�� ��쿡�� �м����� �ʴ´�.
		if ((max_restored_prob - RestoredEJ->mresult[i].prob) > CutoffThreshold
			|| RestoredEJ->mresult[i].prob <= PROBcLOG_ALMOST_ZERO) 
		{
			break;
		}
#ifdef DEBUG
		fprintf(stderr, "������ ���� = %s\n���� �� = %d\n���� ��ȣ = %d\n", rest_ej, num_splitchar, ej_num);
#endif
		// �ʱ�ȭ
		strcpy( splitchar[0], BOW_SYL_2);	 // ���� ���� ����
		strcpy( splitchar[1], BOW_SYL_1);	 // ���� ���� ����

		strcpy( splitchar[num_splitchar+2], EOW_SYL);	 // ���� �� ����

		///////////////////////////////////////////////////////////////////////////////
		MAtRESULTS *result_bfs;
		
		result_bfs = (MAtRESULTS *)malloc( sizeof( MAtRESULTS));
		assert( result_bfs != NULL);
		
		result_bfs->count = 0;

		// ���� �� ���¸� ����� (trigram)
		// ���� �� ���¸� ����� (bigram) -> trigram �𵨷� ����
		bigram_BFS( Rsc, ej_num, splitchar, num_splitchar,
				RestoredEJ->mresult[i].prob, // Ȯ��
				CutoffThreshold,
				BeamSize,
				result_bfs); //�������

#ifdef DEBUG
		fprintf( stderr, "result_bfs->count = %d\n", result_bfs->count);
#endif
		// ��� ������ ����� ����
		// ���������� �ϳ� �� (���� �� ��������)
		for (j = 0; j < result_bfs->count; j++)
		{
			if (j >= BeamSize) 
				break;
			
			char *tags[1024];
			int num_tags = 0;

#ifdef DEBUG			
			fprintf( stderr, "%s\n", result_bfs->mresult[j].str);
#endif
			num_tags = getSyllableTags( result_bfs->mresult[j].str, tags); // �±׸� �ϳ��� ��
		
			// ��� ���� (���� ������ �±�� ���� ���¼� ���� ����� ��ȯ)
			if ( !ConvertSyll2Morph( splitchar, tags, num_splitchar, DELIMITER, morph_result)) 
			{
				// ����->���¼� ��� ����
				assert( 0);
				continue; // ���� �߻��� ���
			}
#ifdef DEBUG
			fprintf( stderr, "%s\t%e\n", morph_result, result_bfs->mresult[j].prob); // �м� ���
#endif		
			// ��� ���� (Ȯ�� + ���¼�/ǰ�� ��)	
			result_s->mresult[result_s->count].str = strdup( morph_result);
			result_s->mresult[result_s->count].prob = result_bfs->mresult[j].prob;
			result_s->count++;

		} // end of for j
		
		// �޸� ����
		ma_Free( result_bfs);
	} // end of for i

	// ����
	qsort( result_s->mresult, result_s->count, sizeof( MAtRESULT), ma_CompareProb);

	// ��� ����
	int num_result = arrange_result_s( result_s, Result, CutoffThreshold);
	
	// �޸� ����
	ma_Free( result_s);

	return num_result;
}
