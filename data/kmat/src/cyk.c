//#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <limits.h> // LONG_MAX
#include <assert.h>
#include <string.h>

#include "kmat_def.h" // DELIMITER
#include "ma.h"
#include "probtool.h"
#include "triangular_matrix.h"

///////////////////////////////////////////////////////////////////////////////
static char *strRstr
(
	char		*String,
	const char	*Find
)
{
	size_t stringlen, findlen;
	char *cp;

	findlen = strlen( Find);
	stringlen = strlen( String);

	if (findlen > stringlen)
		return NULL;

	for (cp = String + stringlen - findlen; cp >= String; cp--) 
	{
		if (strncmp( cp, Find, findlen) == 0)
			return cp;
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// ���� ���¼� �м� ���
// CYK �˰���
int cyk_ExecM
(
	MAtRSC_M		*Rsc,			// [input] ���¼� ���� �� ���ҽ�
	MAtRESULTS		*SubstrInfo,	// [input] �κ� ���ڿ��� ���� ����
	int 			NumCell,		// [input] �ﰢǥ�� ���� ��
	int			EjIndex,		// [input] �� ��° �����ΰ�?
	char			**Substr,		// [input] �κ� ���ڿ�
	int			NumSyll,		// [input] ���� ��
	double		PhoneticProb,	// [input] ���� ���� Ȯ��
	double		CutoffThreshold,	// [input] �ƿ��� �Ӱ谪
	double		AbsThreshold,	// [input] ���� �Ӱ谪
	MAtRESULTS		*Result		// [output] ��� ����
)
{
	PROBtFST *rsc_trans = Rsc->transition;	// ���� Ȯ��
	PROBtFST *rsc_morph = Rsc->morph;		// ���¼� Ȯ��
	int i, j, k, l;
	int cur_tab; // ����
	int front_tab; // ����
	int total_tab; // ���� + ����

	MAtRESULTS *table; // �κ� ��� ����

	double trans_prob, trans_prob2; // ����Ȯ��
	double morph_prob; // ���¼� Ȯ��

	static double max_prob; // �ִ� Ȯ�� (static ���� ����)
	double cur_prob; // ���� Ȯ��
	
	double table_max[NumCell]; // table�� �� ���ҿ��� ���� ū Ȯ������ ����
	
	table = (MAtRESULTS *)malloc( sizeof( MAtRESULTS) * NumCell);
	assert( table != NULL);

	// �ʱ�ȭ
	for (i = 0; i < NumCell; i++)
	{
		table[i].count = 0;
		table_max[i] = -LONG_MAX;
	}
	
	// ù��° ������ �ƴϸ�
	// ���ݱ����� �ִ� Ȯ������ �����ϰ� ����
	if (EjIndex == 0) max_prob = -LONG_MAX;

	/*
	��ȭ����
	��(0), MAG, -8.18
	��(0), NNG, -9.88
	��(0), NNP, -8.24
	��(0), NR, -8.00
	��ȭ(1), NNG, -7.32
	ȭ(4), NNG, -8.66
	ȭ(4), NNP, -11.50
	ȭ(4), XSN, -2.91
	ȭ��(5), NNG, -10.32
	��(7), MM, -3.72
	��(7), NNB, -9.56
	��(7), NNG, -6.35
	��(7), NNP, -6.30
	����(8), NNG, -7.65
	��(9), NNB, -10.86
	��(9), NNG, -7.83
	��(9), NNP, -7.94
	*/

	t_TAB pos;

	const char delim[2] = {DELIMITER, 0};

#ifdef DEBUG
	fprintf(stderr, "\nwelcome to new cyk_m--------\n");
	fprintf(stderr, "����Ȯ�� = %f\n", PhoneticProb);
	
#endif

	// base case
	// ������ ����� ���¼ҵ�(���� ��ġ�� �ִ� ���¼Ҹ�)�� ǰ��� Ȯ���� �˾Ƴ��� ���̺� ����
	for (i = 0; i < NumSyll; i++)
	{
		double cur_max = -LONG_MAX;
		
		TabPos1to2( i, NumSyll, &pos);
			
		if (pos.x != 0) // ������ ���ۺκ��� �ƴϸ�
		{
			continue; // do nothing
		}
		////////////////////////////////////////////////////
		// ��� ǰ�翡 ����
		for (j = 0; j < SubstrInfo[i].count; j++)
		{
#ifdef DEBUG
			fprintf(stderr, "%s(%d), %s, %.2lf\n", Substr[i], i, SubstrInfo[i].mresult[j].str, SubstrInfo[i].mresult[j].prob);
#endif
			// ���� ó�� �±׿��� ���Ἲ �˻�
			// ���� �Ұ����ϸ�
			// P(ùǰ�� | t0)
//			if ((trans_prob = prob_GetFSTProb2( rsc_trans, SubstrInfo[i].mresult[j].str, (char *)BOW_TAG_1)) <= PROBcLOG_ALMOST_ZERO) 
//			{
//#ifdef DEBUG
//				fprintf(stderr, "�Ұ� %s -> %s\n", BOW_TAG_1, SubstrInfo[i].mresult[j].str); //iter->first.c_str());
//#endif
//				continue;
//			}
//			
//#ifdef DEBUG
//			fprintf(stderr, "���� Ȯ�� P(%s -> %s) = %lf\n", BOW_TAG_1, SubstrInfo[i].mresult[j].str, trans_prob);
//#endif
			// ���¼� �߻� Ȯ��
			//morph_prob = prob_GetFSTProb3( rsc_morph, Substr[i], (char *)BOW_SYL_1, SubstrInfo[i].mresult[j].str); // P(m_i | m_i-1, t_i)
			morph_prob = prob_GetFSTProb2( rsc_morph, Substr[i], SubstrInfo[i].mresult[j].str); // P(m_i | t_i)
#ifdef DEBUG
			fprintf(stderr, "���¼�Ȯ�� P(%s | %s, %s) = %lf\n", Substr[i], (char *)BOW_SYL_1, SubstrInfo[i].mresult[j].str, morph_prob);
#endif
			/////////////////////////////////////////////////////
			if (pos.y == NumSyll) // ���� ��ü�� �ϳ��� ���¼��̸�
			{
				// ���� �� �±׿��� ���Ἲ �˻�
				// ���� �Ұ����ϸ�
				// P(������ǰ�� | ����ǰ��)
//				if ((trans_prob2 = prob_GetFSTProb2( rsc_trans, (char *)EOW_TAG, SubstrInfo[i].mresult[j].str)) <= PROBcLOG_ALMOST_ZERO) 
//				{
//#ifdef DEBUG
//					fprintf(stderr, "�Ұ� %s -> %s\n", SubstrInfo[i].mresult[j].str, EOW_TAG);
//#endif
//					continue;
//				}
//
//#ifdef DEBUG
//				fprintf(stderr, "���� Ȯ�� P(%s -> %s) = %lf\n", SubstrInfo[i].mresult[j].str, EOW_TAG, trans_prob2);
//#endif
				// ���� Ȯ�� + ����Ȯ�� + ����� Ȯ�� + ����Ȯ��
				cur_prob = morph_prob + /*trans_prob +*/ PhoneticProb /*+ trans_prob2*/;
				
				// �ִ� Ȯ�� ���� (��ü�� �ϳ��� ���¼��̹Ƿ�)
				if (cur_prob > max_prob) max_prob = cur_prob;
			}
			///////////////////////////////////////////////////////
			else // ������ ���ۺκ� and ��ü�� �ϳ��� ���¼Ұ� �ƴ�
			{
				// ���� Ȯ�� + ����Ȯ�� + ����� Ȯ��
				cur_prob = morph_prob + /*trans_prob +*/ PhoneticProb; 
			}
			
			if (cur_prob > cur_max)
			{
				cur_max = cur_prob;
			}

			// cut-off�� �������� �˻�
			if (CutoffThreshold > 0 && max_prob - cur_prob > CutoffThreshold) 
			{
#ifdef DEBUG
				fprintf( stderr, "cutoffed\nmax_prob %lf - cur_prob %lf = %lf > threshold %lf\n\n", max_prob, cur_prob, max_prob-cur_prob, CutoffThreshold);
#endif
				continue; // ���⼭ ����
			}

			// table�� ����
			// table[�ε���][���¼�/ǰ��] = Ȯ��
			//table[it->first][Substr[it->first]+delimiter+iter->first] = cur_prob;
			char temp_result[1024];
			sprintf( temp_result, "%s%c%s", Substr[i], DELIMITER, SubstrInfo[i].mresult[j].str);
			//table[i][temp_result] = cur_prob;
			
			int count = table[i].count;
			table[i].mresult[count].str = strdup( temp_result);
			table[i].mresult[count].prob = cur_prob;
			table[i].count++;
			
#ifdef DEBUG
			fprintf(stderr, "table[%d] : %s/%s : %lf\n\n", i, Substr[i], SubstrInfo[i].mresult[j].str, cur_prob);
#endif
		} // for (j = 0; j < SubstrInfo[i].count; j++)
		
		table_max[i] = cur_max; // �ִ밪�� ����
		
#ifdef DEBUG
		fprintf(stderr, "table_max[%d] = %lf\n\n", i, cur_max);
#endif

		// �ִ밪�� Ȯ������ ���̰� ���� ���� ����� ����
		if ( table[i].count > 0) // ����� �� �־��
		{
			// ����
			qsort( table[i].mresult, table[i].count, sizeof( MAtRESULT), ma_CompareProb);
		
			for (j = 1; j < table[i].count; j++) // �Ǿ� Ȯ���� ���� ���� ���̹Ƿ� 0�� ����
			{
				if (cur_max - table[i].mresult[j].prob > CutoffThreshold)
				{
					int k;
				
					for (k = j; k < table[i].count; k++) // ������� ���������� ����
					{
#ifdef DEBUG
						fprintf(stderr, "%s ������\n", table[i].mresult[k].str);
#endif
						free( table[i].mresult[k].str);
					}
					table[i].count = j;
					break;
				}
			}
		} // if ( table[i].count > 0)
	} // for (i = 0; i < NumCell; i++)

#ifdef DEBUG
	fprintf(stderr, "�ʱ�ȭ ���\n\n");
#endif
	//////////////////////////////////////////////////////////////////////////////
	char result[1000];
	char *prev_last_pos;
	char *prev_last_morph;
	char prev_result[1000];
	double cur_max;

	//for (j = 2; j <= NumSyll; j++) {
	//	for (i = j-1; i >= 1; i--) {

	// T(0, i) : ����
	// T(i, j) : ����
	// T(0, j) : ��ģ ���
	for (i = 1; i < NumSyll; i++) 
	{
		for (j = i+1; j <= NumSyll; j++) 
		{
			cur_tab = TabPos2to1( i, j, NumSyll); // ����
			front_tab = TabPos2to1( 0, i, NumSyll); // ����

			// �պκ� + ����κ�
#ifdef DEBUG
			fprintf( stderr, "\n%s + %s\n", Substr[front_tab], Substr[cur_tab]);
#endif
			// ����κ��� ������ ��ϵǾ� ���� ������
			if (SubstrInfo[cur_tab].count <= 0)
			{
#ifdef DEBUG
				fprintf( stderr, "[%s]�� ��� ����\n", Substr[cur_tab]);
#endif
				continue;
			}
			
			// �պκ��� ��м� ����� ������
			if (table[front_tab].count <= 0)
			{
#ifdef DEBUG
				fprintf( stderr, "[%s]�� ��� ����\n", Substr[front_tab]);
#endif
				continue;
			}

			// ���� + ����
			total_tab = TabPos2to1( 0, j, NumSyll);
			
			// �̹� ����� ����+����κп� ���� �ִ� Ȯ���� (�̹� ����Ǿ� ���� �ʴٸ� -LONG_MAX)
			cur_max = table_max[total_tab];
	
#ifdef DEBUG
			fprintf( stderr, "cur_max = %lf\n", cur_max);
#endif

			// �պκ� (��� �м� ����� ����)
			//for (iter = it2->second.begin(); iter != it2->second.end(); ++iter)
			for (l = 0; l < table[front_tab].count; l++)
			{
				strcpy( prev_result, table[front_tab].mresult[l].str);
							
				// ���� �м��� ������ ǰ��
				prev_last_pos = strRstr( prev_result, delim);
				*prev_last_pos = 0;
				prev_last_pos++;
				
				// ���� �м��� ������ ���¼�
				prev_last_morph = strRstr( prev_result, "++");
				if (prev_last_morph == NULL) 
				{
					prev_last_morph = strRstr( prev_result, "+");
					if (prev_last_morph == NULL) prev_last_morph = prev_result;
					else prev_last_morph++;
				}
				else prev_last_morph++;
				
				// ����κ� (��� ǰ�翡 ����)
				for (k = 0; k < SubstrInfo[cur_tab].count; k++)
				{

					// ������ ������ ������ �����. (���߿� �����ؾ� ��)
					if (table[total_tab].count >= KMATcMAX_MA_RESULT_NUM-1) break;

#ifdef DEBUG
					fprintf(stderr, "\nprev (m,t): %s, %s\n", prev_last_morph, prev_last_pos);
					fprintf(stderr, "cur	(m,t): %s, %s\n", Substr[cur_tab], SubstrInfo[cur_tab].mresult[k].str);
#endif 			 

					// ���� ���ɼ� �˻�
					// ���� ǰ����� ���Ἲ �˻�
					// ����Ȯ�� : ���� �м��� ������ ǰ�� + ���� ���¼��� ǰ��
					// P( iter2->first | prev_last_pos)
					if ((trans_prob = prob_GetFSTProb2( rsc_trans, SubstrInfo[cur_tab].mresult[k].str, prev_last_pos)) <= PROBcLOG_ALMOST_ZERO) // ���� �Ұ����ϸ� // Ȯ���� �� ��!
					{
#ifdef DEBUG
						fprintf(stderr, "�Ұ� %s -> %s\n", prev_last_pos, SubstrInfo[cur_tab].mresult[k].str);
#endif
						continue;
					}
#ifdef DEBUG
					fprintf(stderr, "���� Ȯ�� P(%s -> %s) = %lf\n", prev_last_pos, SubstrInfo[cur_tab].mresult[k].str, trans_prob);
#endif
					// �����ؼ� ���� �� ���� �±�
					if (strcmp( prev_last_pos, SubstrInfo[cur_tab].mresult[k].str) == 0)
					{
						if (strcmp( prev_last_pos, "SL") == 0 
								|| strcmp(prev_last_pos, "SH") == 0 
								|| strcmp(prev_last_pos, "SN") == 0
								|| strcmp(prev_last_pos, "SE") == 0)
						{
							continue;
						}
					}

					// ���¼� Ȯ��
					morph_prob = prob_GetFSTProb3( rsc_morph, Substr[cur_tab], prev_last_morph, SubstrInfo[cur_tab].mresult[k].str); // // P(m_i | m_i-1, t_i)
#ifdef DEBUG
					fprintf(stderr, "���¼�Ȯ�� P(%s | %s, %s) = %lf\n", Substr[cur_tab], prev_last_morph, SubstrInfo[cur_tab].mresult[k].str, morph_prob);
#endif 							 
					
					// ������� �� ���� ���ᰡ���� �����
					// �м���� �� Ȯ�� ����
					// ���� �м� ��� + ���� ���¼�/ǰ��
					sprintf( result, "%s+%s%c%s", table[front_tab].mresult[l].str, Substr[cur_tab], DELIMITER, SubstrInfo[cur_tab].mresult[k].str);
					
					// ���� ���ΰ�?
//					if (j == NumSyll) 
//					{
//						// ������ ǰ����� ���Ἲ �˻�
//						// P( EOW_TAG | iter2->first)
//						if ((trans_prob2 = prob_GetFSTProb2( rsc_trans, (char *)EOW_TAG, SubstrInfo[cur_tab].mresult[k].str)) <= PROBcLOG_ALMOST_ZERO) 
//						{
//#ifdef DEBUG
//							fprintf(stderr, "�Ұ� %s -> %s\n", SubstrInfo[cur_tab].mresult[k].str, EOW_TAG);
//#endif
//							continue;
//						}
//#ifdef DEBUG
//						fprintf(stderr, "���� Ȯ�� P(%s -> %s) = %lf\n", SubstrInfo[cur_tab].mresult[k].str, EOW_TAG, trans_prob2);
//#endif
//						// ������ ���¼� Ȯ���� ���� �ȵ�
//							
//						// ���� �м� Ȯ�� + ���� Ȯ�� + ���� Ȯ�� + (������ ǰ��� EOW����) ���� Ȯ�� 
//						cur_prob = table[front_tab].mresult[l].prob + trans_prob + morph_prob + trans_prob2;
//#ifdef DEBUG
//						fprintf(stderr, "prob = %lf (max prob = %lf)\n", cur_prob, cur_max);
//#endif
//						if (cur_prob > cur_max) cur_max = cur_prob; // �ִ�Ȯ�� ����
//					}
//					else // ���� ���� �ƴϸ�
					{
						// ���� �м� Ȯ�� + ���� Ȯ�� + ���� Ȯ��
						cur_prob = table[front_tab].mresult[l].prob + trans_prob + morph_prob;

						if (cur_prob > cur_max) cur_max = cur_prob; // �ִ�Ȯ�� ����
#ifdef DEBUG
						fprintf(stderr, "cur prob = %lf (max prob = %lf)\n", cur_prob, cur_max);
#endif
					}
						
					// cut-off�� �������� �˻�
					if (CutoffThreshold > 0 && cur_max - cur_prob > CutoffThreshold)
					{
#ifdef DEBUG
						fprintf(stderr, "cutoffed1\ncur_max %lf - cur_prob %lf = %lf > threshold %lf\n\n", cur_max, cur_prob, cur_max-cur_prob, CutoffThreshold);
#endif
						continue; // ���⼭ ����
					}

					// �������� ����Ͽ� ���� threshold ���� Ȯ������ ������
					#ifdef SYLLABLE_ANALYSIS // �ڿ��� ���� ���� �м��� �ϴ� ��츸 �˻�
					if (cur_prob < AbsThreshold)
					{
#ifdef DEBUG
						fprintf(stderr, "cutoffed3\ncur_prob = %lf < AbsThreshold = %lf\n", cur_prob, AbsThreshold);
#endif
						continue;
					}
					#endif

					// �ӵ� ��� ����� �⿩, but �����Ǵ� ������� ���� ���� �ִ�.
					if (cur_max - cur_prob > 15) // 10�� �Ẹ�⵵ �߾���
					{
#ifdef DEBUG
						fprintf(stderr, "cutoffed2\n\n");
#endif
						continue;
					}

					// ���� (��� �� Ȯ��)
					table[total_tab].mresult[table[total_tab].count].str = strdup( result);
					table[total_tab].mresult[table[total_tab].count].prob = cur_prob;
					table[total_tab].count++;
					/**/assert( table[total_tab].count < KMATcMAX_MA_RESULT_NUM);

					table_max[total_tab] = cur_max;

#ifdef DEBUG
					if (j == NumSyll) fprintf(stderr, "�ϰ�[%d at %d]: ", table[total_tab].count-1, total_tab);
					else fprintf(stderr, "�߰� : ");
							
					fprintf(stderr, "[%s\t%f]\n", result, cur_prob);
					fprintf(stderr, "\n");
#endif
				} // end of for ����κ� (��� ǰ�翡 ����)
			} // end of for �պκ� (��� �м� ����� ����)
			
			// ����
			// �̰��� �ϰ� �Ǹ� ����� ���� �پ��� (cutoff�� ����), �м� �ð��� �ణ ����ȴ�.
			qsort( table[total_tab].mresult, table[total_tab].count, sizeof( MAtRESULT), ma_CompareProb);
			
		} // end of for (i = j-1; i >= 1; i--)
	} // end of for (j = 2; j <= NumSyll; j++)

	// (��ü ������ ����) ���� �м� ��� /////////////////////////////////////////////
	cur_tab = TabPos2to1( 0, NumSyll, NumSyll);
	
	// Ȯ������ ���� ���ĵǾ� �����Ƿ� ù��° ����� ���� ���� Ȯ���� �����ٰ� ����
	max_prob = table_max[cur_tab];

	for (i = 0; i < table[cur_tab].count; i++)
	{
		assert( max_prob >= table[cur_tab].mresult[i].prob);
		
		if (CutoffThreshold > 0 && max_prob - table[cur_tab].mresult[i].prob > CutoffThreshold) // �Ӱ谪���� ���� ��
		{
#ifdef DEBUG
			fprintf(stderr, "cutoffed4\n");
#endif
			break; // �� �̻� �� �� ���� ���⼭ ����
		}

		// ��� ���� (Ȯ�� + �м����)
		// �� �̻� ������ ������ ������ ����
		if (Result->count >= KMATcMAX_MA_RESULT_NUM-1) break;
		
		Result->mresult[Result->count].str = strdup( table[cur_tab].mresult[i].str );
		Result->mresult[Result->count].prob = table[cur_tab].mresult[i].prob;
		Result->count++;
	}
	
	// �޸� ����
	for (i = 0; i < NumCell; i++)
	{
		for (j = 0; j < table[i].count; j++)
		{
			if (table[i].mresult[j].str)
			{
				free( table[i].mresult[j].str);
				table[i].mresult[j].str = NULL;
			}
		}
	}
	free( table);

	return 1;
}
