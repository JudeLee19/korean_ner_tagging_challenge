//#define DEBUG

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "ma.h"
#include "dafst.h"
#include "bin2txt.h"
#include "binio.h"
#include "kmat_def.h"
#include "kmat.h"

///////////////////////////////////////////////////////////////////////////////
void ma_CloseE
(
	MAtRSC_E *Rsc
)
{
	if (Rsc == NULL) return;
	
	if (Rsc->fst)
	{
		fst_Close( Rsc->fst);
		Rsc->fst = NULL;
	}
	
	b2t_Close( Rsc->info);
	
	bin_Close( Rsc->freq);

	free( Rsc);
	Rsc = NULL;
}

///////////////////////////////////////////////////////////////////////////////
MAtRSC_E *ma_OpenE
(
	char	*RMEJ_FST_Path,
	char	*RMEJ_FST_INFO_Path,
	char	*RMEJ_FST_FREQ_Path
)
{

	MAtRSC_E *rsc = (MAtRSC_E *) malloc( sizeof( MAtRSC_E));
	if (rsc == NULL) return NULL;
	
	fprintf( stderr, "\tLoading RMEJ FST.. [%s]", RMEJ_FST_Path);
	if ((rsc->fst = fst_Open( RMEJ_FST_Path, NULL)) == NULL)
	{
		fprintf(stderr, "Load failure [%s]\n", RMEJ_FST_Path);
		return NULL;
	}
	fprintf( stderr, "\t[done]\n");
	
	//
	fprintf( stderr, "\tLoading RMEJ information.. [%s]", RMEJ_FST_INFO_Path);
	if ((rsc->info = b2t_Open( RMEJ_FST_INFO_Path)) == NULL)
	{
		fprintf( stderr, "b2t_Open error [%s]\n", RMEJ_FST_INFO_Path);
		return NULL;
	}
	fprintf( stderr, "\t[done]\n");

	//
	fprintf( stderr, "\tLoading RMEJ frequencies.. [%s]", RMEJ_FST_FREQ_Path);
	if ((rsc->freq = bin_Open( RMEJ_FST_FREQ_Path)) == NULL)
	{
		fprintf( stderr, "bin_Open error [%s]\n", RMEJ_FST_FREQ_Path);
		return NULL;
	}
	fprintf( stderr, "\t[done]\n");
	
	//**/fprintf( stderr, "# of RMEJ string = %d\n", b2t_GetStringCount( rsc->info));
	//**/fprintf( stderr, "sizeof(int) = %d, sizeof(long) = %d\n", sizeof(int), sizeof(long));
	//**/fprintf( stderr, "sizeof(int *) = %d, sizeof(long *) = %d\n", sizeof(int *), sizeof(long *));
	
	return rsc;
}

///////////////////////////////////////////////////////////////////////////////
// ���� ���� ���¼� �м�
// return value: �м� ����� ��
int ma_ExecE
(
	MAtRSC_E		*Rsc,
	const char		*Eojeol, 
	MAtRESULTS		*Result
)
{

	int result_num = 0;
	int i, n;
	int index;
	int results[1024];
	int total_freq = 0;

	Result->count = 0; // �ʱ�ȭ

	if ((n = fst_String2Hash( Rsc->fst, (char *) Eojeol, &index)) == (-1)) // ����Ʈ�� ������
	{
		return 0;
	}
	else 
	{
		for (i = 0; i < index; i++) // �������� �м� ����� ���� ���
		{
			results[result_num++] = n++;
		}
		
	}

#ifdef DEBUG		
		fprintf( stderr, "result_num of %s = %d\n", Eojeol, result_num);
#endif

	// ���� �� (Ȯ������ �и�)
	for (i = 0; i < result_num; i++) 
	{
#ifdef DEBUG		
		fprintf( stderr, "[%d] freq = %d\n", i, bin_GetEntryInt( Rsc->freq, results[i]));
#endif
		total_freq += bin_GetEntryInt( Rsc->freq, results[i]);

	}
	
	// �м� ����� ����ŭ ��� ���� (���¼�/ǰ�� �� + Ȯ��)
	for (i = 0; i < result_num; i++) 
	{
		// �м� ��� 
		Result->mresult[Result->count].str = strdup( b2t_GetString ( Rsc->info, results[i]));
		// Ȯ��
		Result->mresult[Result->count].prob = log( (double) bin_GetEntryInt( Rsc->freq, results[i]) / total_freq);
#ifdef DEBUG		
		fprintf( stderr, "[%d] %s %e\n", Result->count, b2t_GetString ( Rsc->info, results[i]), Result->mresult[Result->count].prob);
#endif
		Result->count++;
	}

	// ����
	qsort( Result->mresult, Result->count, sizeof( MAtRESULT), ma_CompareProb);

	return result_num;
}

///////////////////////////////////////////////////////////////////////////////
// ��� ���
// ���ϰ� : �м� ����� ��
int print_result_e
(
	FILE *fp,
	MAtRESULTS *Result
)
{
	int i;

	// �ݺ�
	for (i = 0; i < Result->count; i++)
	{
		// �м� ���, Ȯ��
		fprintf( fp, "\t%s\t%12.11e\n", Result->mresult[i].str, Result->mresult[i].prob);
	}

	return Result->count; // �м��� ����� ��
}
