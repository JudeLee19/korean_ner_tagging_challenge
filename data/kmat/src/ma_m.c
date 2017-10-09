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
// ���ҽ� ����
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
	
	// Ȯ���� �Է�
	// ���� Ȯ��
	rsc->lexical = prob_OpenWithInfoFST( LEXICAL_FST_Path, LEXICAL_INFO_Path, LEXICAL_PROB_Path);
	assert( rsc->lexical != NULL);
	
	// ���� Ȯ��
	rsc->transition = prob_OpenFST( TRANSITION_FST_Path, TRANSITION_PROB_Path);
	assert( rsc->transition != NULL);

	// Ȯ��
	rsc->morph = prob_OpenFST( M_MORPH_FST_Path, M_MORPH_PROB_Path);
	assert( rsc->morph != NULL);
	
	return rsc;
}

///////////////////////////////////////////////////////////////////////////////
// ��� �κ� ���ڿ��� �̸� �������� ã�´�.
// ���Ŀ� ȿ�������� ������ �ʿ䰡 ����
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

	// �ݵ�� �ʱ�ȭ�ؾ� ��
	// ����� �� ���� ������ �ٽ� ������ �ϱ� ���� (�ʱ�ȭ�� ���� ������ ���� ����Ÿ�� ���� ���� �߻�)

	// �κ� ���ڿ� ����
	for (k = i = 0; i < len; i++) 
	{
		for (j = i; j < len; j++, k++) 
		{
			strncpy( str, SrcStr+i*2, (j-i+1)*2);
			str[(j-i+1)*2] = 0;
//#ifdef DEBUG
//			fprintf(stderr, "[%d] %s\n", k, str);
//#endif
			// �κ� ���ڿ��� �������� ã�´�.
			if ((n = fst_String2Hash( lexical->fst, str, &index)) != (-1))
			{
				// ������
				// ���� �� �ִ� ��� ǰ�縦 ã�´�.
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
// result_m�� ����� ����� �����Ͽ� analyzed_result_m�� ����
// ���ϰ� : �м� ����� ��
static int arrange_result_m
(
	MAtRESULTS	*Result_in,
	MAtRESULTS	*Result_out,
	double	CutoffThreshold
)
{
	int i;
	
	// ��� ��ȯ
	char result_str[MAX_WORD]; // FIL�� ������ ���

	if (Result_in->count < 1) return 0; // ����� ������ ����

	// Ȯ�� normalization
	double max_prob = Result_in->mresult[0].prob; // �ְ� Ȯ����
	
	Result_out->count = 0; // �ʱ�ȭ
	//assert( Result_out->count == 0);

	// ��� �м� ����� ����
	// res_itr->first : Ȯ����
	// res_itr->second : ���¼� �м����
	for (i = 0;	i < Result_in->count; i++)
	{
		
		assert( max_prob >= Result_in->mresult[i].prob);
		
		// cut-off�� �������� �˻�
		// �ְ� Ȯ���� ���� ������� �αװ��� ���� ����ġ�̻��̸� ����
		if (CutoffThreshold > 0 && (max_prob - Result_in->mresult[i].prob) > CutoffThreshold) 
		{
			// ���⼭���� ������������ ������ �ʿ���� (������ ����� Ȯ������ �� �����Ƿ�)
			break;
		}
		
		if (strstr( Result_in->mresult[i].str, "/NA") != NULL) continue; // �м� ����� NA�� ������ �ȵ�

		// ������ �±׿��� ���缺 �˻�
/*	�����ؾ���	if ( !(num_morph = check_morpheme_result( Result_in->mresult[i].str, str_syl_tag_seq, DELIMITER))) 
		{
			continue; // do nothing
		}
*/		
		// ������� �� ���� �����ؾ� ��
		convert_str_origin_array( Result_in->mresult[i].str, result_str); // ������� FIL�� ����
		Result_out->mresult[Result_out->count].str = strdup( result_str );
		Result_out->mresult[Result_out->count].prob = Result_in->mresult[i].prob; // �α� Ȯ��
		Result_out->count++;
	} // end of for

	return Result_out->count; // �м��� ����� ��
}

///////////////////////////////////////////////////////////////////////////////
// Ȯ���� ���¼� �м� (���¼� ����)
int ma_ExecM
(
	MAtRSC_M	*Rsc,
	MAtRESULTS	*RestoredEJ,
	int		IsFirst,	// ���� ���ۺ��ΰ�?
	int		IsLast,	// ���� ������ΰ�?
	MAtRESULTS	*Result, // [output]
	double	CutoffThreshold
)
{
	int num_syll; // ���� ��
	t_TAB position; // ���ڿ��� ��ġ��
	
	MAtRESULTS *substr_info; // �κ� ���ڿ� ����
	
	int num_cell; // �ﰢǥ�� ���� ��
	int i, j, k;
	
	MAtRESULTS *result_m;
	char **sub_str; // �κ� ���ڿ� ����
	
	int num_result = 0;
	
	// �޸� �Ҵ�		
	result_m = (MAtRESULTS *) malloc( sizeof (MAtRESULTS));
	assert( result_m != NULL);
	
	result_m->count = 0;
	
	double max_restored_prob = RestoredEJ->mresult[0].prob; // ù ���� ���� Ȯ��

	// ��� ������ ������ ���� �ݺ�
	int ej_num = 0; // ������ ������ ��ȣ
	for (i = 0; i < RestoredEJ->count; i++, ej_num++)
	{
		// ���ڿ� ���� (���� ��)
		num_syll = (int) strlen( RestoredEJ->mresult[i].str) / 2; 

#ifdef DEBUG
		fprintf( stderr, "���� ũ�� = %d (����)\n", num_syll);
#endif
		
		// ���� ���� 15���� ���� ������ ù��° ������ ������ ���ؼ��� ����
		if (num_syll >= 15) 
		{
			if (i) break;
		}

		// ���� ���� Ȯ������ �ʹ� ���� ��쳪 0�� ��쿡�� �м����� �ʴ´�.
		if ( RestoredEJ->mresult[i].prob <= PROBcLOG_ALMOST_ZERO 
				 || (CutoffThreshold > 0.0 && (max_restored_prob - RestoredEJ->mresult[i].prob) > 3/*CutoffThreshold*/) ) 
		{
#ifdef DEBUG
			if (RestoredEJ->mresult[i].prob <= PROBcLOG_ALMOST_ZERO) 
				fprintf(stderr, "���� Ȯ���� = 0\n");
			else fprintf(stderr, "�ʹ� ���� ���� Ȯ����\n");
#endif
			break;
		}

		if (num_syll > 30) // 30���� �ʰ��̸� CYK �˰����� �ʹ� ��ȿ�����̹Ƿ� �м� ���� ����
		{
#ifdef DEBUG
			fprintf( stderr, "���¼� ���� �м� ����\n");
#endif
			continue;
		}

		// �ʱ�ȭ
		setpos( &position, 0, num_syll); // ��ġ ����
		
		num_cell = TabNum( num_syll);
		
		substr_info = (MAtRESULTS *)malloc( sizeof( MAtRESULTS) * num_cell);
		assert( substr_info != NULL);
		
		sub_str = (char **)malloc( sizeof( char *) * num_cell);
		assert( sub_str != NULL);
		
		// �ʱ�ȭ
		for (j = 0; j < num_cell; j++)
		{
			substr_info[j].count = 0;
		}
		
		// �κ� ���ڿ� ����
		// ��� : sub_str
		GetAllSubstring( RestoredEJ->mresult[i].str, num_syll, sub_str);
		
		// �κ� ���ڿ� ����
		// ��� �κ� ���ڿ��� �̸� �������� ã�´�.
		// ��� : substr_info (�� �κ� ���ڿ��� ���� �� �ִ� ǰ��� Ȯ���� �� ����)
		get_all_possible_pos_from_substring( num_syll, 
								RestoredEJ->mresult[i].str, 
								Rsc->lexical,
								substr_info);

		// ������� ���� ���
#ifdef DEBUG
		fprintf(stderr, "������ ���� : %s\t%e\n", RestoredEJ->mresult[i].str, RestoredEJ->mresult[i].prob);
#endif

		// ���¼� �м� ���
		cyk_ExecM( Rsc,
				substr_info, 
				num_cell,
				ej_num, // ���� ��ȣ
				sub_str,
				num_syll, 
				RestoredEJ->mresult[i].prob, // �α� Ȯ�� (�ʱⰪ)
				CutoffThreshold,
				log( pow( 1.0e-03, num_syll)),
				result_m);

		// �޸� ����
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
	fprintf( stderr, "���¼� ���� �м� ����� �� = %d\n", result_m->count);
#endif

	if (result_m->count > 0) // ���¼� ���� �м� ����� ������
	{
		// ����
		qsort( result_m->mresult, result_m->count, sizeof( MAtRESULT), ma_CompareProb);

		// ��� ����
		num_result = arrange_result_m( result_m, Result, CutoffThreshold);
	}
	
	// �޸� ����
	ma_Free( result_m);

	return num_result;
}
