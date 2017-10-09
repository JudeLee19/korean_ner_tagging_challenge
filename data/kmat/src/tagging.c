//#define DEBUG

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "kmat.h"
#include "tagging.h"
#include "kmat_def.h"

#include "hsplit.h"
#include "probtool.h"
#include "get_morph_tag.h"
#include "viterbi.h"
#include "strvector.h"

#define INTER_TRANS_FST     0
#define INTER_TRANS_PROB    1
#define INTRA_TRANS_FST     2
#define INTRA_TRANS_PROB    3
//#define INTER_TRANS_EJ_PRB  4

const char *tag_RSC_FILES[] = {
	"INTER_TRANS.fst",		// 0
	"INTER_TRANS.prob",	 // 1
	"INTRA_TRANS.fst",		// 2
	"INTRA_TRANS.prob",		// 3
//	"INTER_TRANS_EJ.prb", // 2
	NULL,
};

// ���� ����
// ȭ�ϸ��� ������ ���
char tag_RSC_FILES_PATH[10][100]; // 10

///////////////////////////////////////////////////////////////////////////////
void tag_Close
(
	void	*Rsc
)
{
	TAGtRSC *rsc = (TAGtRSC *)Rsc;
	
	prob_CloseFST( rsc->inter_transition);
	prob_CloseFST( rsc->intra_transition);
	
	if (rsc)
	{
		free( rsc);
		rsc = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
void *tag_Open
(
	char	*RscPath
)
{
	TAGtRSC *rsc; // ���ҽ�
	int i;
	char rsc_path[300];
	
	strcpy( rsc_path, RscPath);
		
	// ���ҽ� ��� ���� (��� + ���ϸ�)
	for (i = 0; tag_RSC_FILES[i]; i++)
	{
		if (rsc_path[0])
		{
			int len = (int) strlen( rsc_path);

#ifdef WIN32 // ��������
			if (rsc_path[len-1] != '\\')
			{
				rsc_path[len] = '\\';
				rsc_path[len+1] = 0;
			}
#else // ���н��� ������
			if (rsc_path[len-1] != '/')
			{
				rsc_path[len] = '/';
				rsc_path[len+1] = 0;
			}
#endif
			sprintf( tag_RSC_FILES_PATH[i], "%s%s", rsc_path, tag_RSC_FILES[i]);
		}
		else
		{
			sprintf( tag_RSC_FILES_PATH[i], "%s", tag_RSC_FILES[i]);
		}
	}
	
	// ���ҽ�
	rsc = (TAGtRSC *) malloc( sizeof( TAGtRSC));

	// Ȯ���� �Է�
	// �ܺ����� Ȯ��
	rsc->inter_transition = prob_OpenFST( tag_RSC_FILES_PATH[INTER_TRANS_FST], tag_RSC_FILES_PATH[INTER_TRANS_PROB]);
	
	assert( rsc->inter_transition != NULL);

	// �������� Ȯ��
	rsc->intra_transition = prob_OpenFST( tag_RSC_FILES_PATH[INTRA_TRANS_FST], tag_RSC_FILES_PATH[INTRA_TRANS_PROB]);
	assert( rsc->intra_transition != NULL);
	
	return (void *)rsc;
}

///////////////////////////////////////////////////////////////////////////////
// Ȯ���� ���¼� �м�
// ���ϰ� : 0 = �м���� ����, 1 = �м� ��� ����
// input_ej : �Է� ����
// analyzed_result : �м� ��� (Ȯ������ �α�)
int tag_Exec
(
	void		*Rsc,				// [input] ���ҽ�
	MAtRESULTS	*MorphAanlyzedResult,	// [input] ���¼� �м� ���
	int		NumWord,			// [input] ������ ��
	int		*StateSeq			// [output] �±� ��� (���¼� �м� ��� ��ȣ)
) 
{
	TAGtRSC *rsc = (TAGtRSC *)Rsc;


	ViterbiSearch2gram( rsc->intra_transition,
					rsc->inter_transition,
					MorphAanlyzedResult,
					NumWord,
					StateSeq);

	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// �±� ��� ���
void tag_PrintResult
(
	FILE		*Fp,				// [output]
	char		**Words,			// [input] ���峻 ����
	MAtRESULTS	*MorphAanlyzedResult,	// [input] ���¼� �м� ���
	int		NumWord,			// [input] ������ ��
	int		*StateSeq			// [input] �±� ��� (���¼� �м� ����� ��ȣ)
)
{
	int i;
	
	// ��� ��Ÿ��
//	if ( !output_style) 
	{
		// �� ������ ����
		for (i = 0; i < NumWord; i++)
		{
#ifdef DEBUG
			fprintf( Fp, "[%d]\t", StateSeq[i+1]);
#endif
			// ������, �м����
			fprintf( Fp, "%s\t%s\n", Words[i], MorphAanlyzedResult[i+1].mresult[StateSeq[i+1]].str); // ����
		}
	}

	// ������ȹ ��Ÿ��
//	else 
//	{
//		int morph_num = 0; // ���� ���� ���¼� ��
//		int j = 0;
//		SVtSTRVCT *morphs;
//		SVtSTRVCT *tags;
//	
//		morphs = sv_New();
//		tags = sv_New();
//
//		// ��������
//		for (i = 0; i < NumWord; i++) 
//		{
//			// ������
//			fprintf( Fp, "%s\t", Words[i]);
//
//			// �м����
//			morph_num = GetMorphTag( MorphAanlyzedResult[i+1].mresult[StateSeq[i+1]].str, 
//							DELIMITER,
//							morphs,
//							tags);
//
//			// ���¼Ҹ���
//			for (j = 0; j < morph_num; j++) 
//			{
//				if (j) 
//				{
//					fprintf( Fp, " + %s%c%s", morphs->strs[j], DELIMITER, tags->strs[j]); 
//				}
//				else 
//				{
//					fprintf( Fp, "%s%c%s", morphs->strs[j], DELIMITER, tags->strs[j]);
//				}
//			}
//			fprintf( Fp, "\n");
//		}
//		sv_Free( morphs);
//		sv_Free( tags);
//	}
}
