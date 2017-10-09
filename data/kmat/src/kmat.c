#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "kmat.h"
#include "kmat_def.h" // BOW_TAG_1
#include "ma.h"
#include "tagging.h"

typedef struct
{
	MAtRSC	*ma_rsc;	// ���¼� �м�
	TAGtRSC	*tag_rsc;	// �±�
} KMATtRSC;

////////////////////////////////////////////////////////////////////////////////
void *kmat_Open
(
	char	*RSC_Path
)
{
	KMATtRSC *rsc = (KMATtRSC *) malloc( sizeof( KMATtRSC));
	assert( rsc != NULL);
	
	// 7 = EOJEOL_ANALYSIS|MORPHEME_ANALYSIS|SYLLABLE_ANALYSIS
	rsc->ma_rsc = ma_Open( RSC_Path, 7);
	assert( rsc->ma_rsc != NULL);

	rsc->tag_rsc = tag_Open( RSC_Path);
	assert( rsc->tag_rsc != NULL);
	
	return rsc;
}

////////////////////////////////////////////////////////////////////////////////
void kmat_Close
(
	void	*Rsc
)
{
	KMATtRSC *rsc = (KMATtRSC *) Rsc;
	
	if (rsc == NULL) return;
	
	ma_Close( rsc->ma_rsc, 7);
	tag_Close( rsc->tag_rsc);
	
	free( rsc);
	rsc = NULL;
}

////////////////////////////////////////////////////////////////////////////////
void kmat_Exec
(
	void		*Rsc,		// [input] ���ҽ�
	char		**Words,	// [input] ���¼� �м� ���
	int		NumWord,	// [input] ������ ��
	char		**Results	// [output] �±� ���
)
{
	MAtRSC *ma_rsc = ((KMATtRSC *)Rsc)->ma_rsc;
	TAGtRSC *tag_rsc = ((KMATtRSC *)Rsc)->tag_rsc;
	
	MAtRESULTS *ma_results = NULL;	// ���¼� �м� ��� ����
	int state_sequence[KMATcMAX_EJ_NUM]; // ǰ�� �±� ���
	int i;
	
	// �޸� �Ҵ�
	ma_results = ma_New( NumWord+1); // +1 : ���� ���� ��ȣ
	
	// ���¼� �м� ��� �ʱ�ȭ
	// 0��°�� ä���.
	//init_morph_analyzed_result( &ma_results[0]);
	ma_results->count = 1;
	ma_results->mresult[0].str = strdup( BOW_TAG_1);
	ma_results->mresult[0].prob = 1.0;
	
	// �� ������ ���� ���¼� �м�
	for (i = 0; i < NumWord; i++)
	{
		ma_Exec( Words[i], ma_rsc, EOJEOL_ANALYSIS|MORPHEME_ANALYSIS|SYLLABLE_ANALYSIS, &ma_results[i+1]);
	}

	// ǰ���±�
	tag_Exec( tag_rsc, ma_results, NumWord, state_sequence);
	
	// ��� ����
	for (i = 0; i < NumWord; i++)
	{
		Results[i] = strdup( ma_results[i+1].mresult[state_sequence[i+1]].str);
		
		//Results[i] = Postprocessing_by_userdic( ma_results[i+1].mresult[state_sequence[i+1]].str);
	}
	
	// �м� ����� ���� �޸� ����
	ma_FreeAll( ma_results, NumWord+1);
}

////////////////////////////////////////////////////////////////////////////////
void kmat_PrintResult
(
	FILE		*Fp,		// [output]
	char		**Words,	// [input] ���峻 ����
	int		NumWord,	// [input] ������ ��
	char		**Results	// [output] �±� ���
)
{
	int i;
	
	for (i = 0; i < NumWord; i++)
	{
		fprintf( Fp, "%s\t%s\n", Words[i], Results[i]);
	}
}
