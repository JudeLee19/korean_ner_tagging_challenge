//#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "probtool.h"
#include "hsplit.h"
#include "bin2txt.h"
#include "binio.h"
#include "dafst.h"
#include "triangular_matrix.h"
#include "phonetic_recover.h"

typedef struct
{
	char	result[MAX_WORD];
	int	freq;
	double	prob;
} tANAL;

typedef struct
{
	int	 count;
	tANAL	*candi;
	int	 total_freq;
	double	max_prob;
} tCELL;

///////////////////////////////////////////////////////////////////////////////
void phonetic_Close
(
	PHONETICtRSC *Rsc
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
PHONETICtRSC *phonetic_Open
(
	char	*FST_Path,
	char	*FST_INFO_Path,
	char	*FST_FREQ_Path
)
{
	PHONETICtRSC *rsc = (PHONETICtRSC *) malloc( sizeof( PHONETICtRSC));
	if (rsc == NULL) return NULL;

	fprintf( stderr, "\tLoading phonetic FST.. [%s]", FST_Path);
	if ((rsc->fst = fst_Open( FST_Path, NULL)) == NULL)
	{
		fprintf(stderr, "Load failure [%s]\n", FST_Path);
		return NULL;
	}
	fprintf( stderr, "\t[done]\n");

	fprintf( stderr, "\tLoading phonetic information.. [%s]", FST_INFO_Path);
	rsc->info = b2t_Open( FST_INFO_Path);

	if (rsc->info == NULL)
	{
		fprintf( stderr, "b2t_Open error [%s]\n", FST_INFO_Path);
		return NULL;
	}
	fprintf( stderr, "\t[done]\n");

	fprintf( stderr, "\tLoading phonetic frequencies.. [%s]", FST_FREQ_Path);
	rsc->freq = bin_Open( FST_FREQ_Path);
	if ( rsc->freq == NULL)
	{
		fprintf( stderr, "bin_Open error [%s]\n", FST_FREQ_Path);
		return NULL;
	}
	fprintf( stderr, "\t[done]\n");

	return rsc;
}

///////////////////////////////////////////////////////////////////////////////
//static void *get_int_Callback
//(
//	 void *mmap_root,
//	 int index
//)
//{
//	 int *contents = (int *) mmap_root;
//	 return &(contents[index]);
//}

///////////////////////////////////////////////////////////////////////////////
#define DEFAULT_CELLNUM 100

typedef struct {
	int	from; // ������ġ
	int	len;	// ���ڿ� ����
	int	hash; // hash value
} tTCELL;

typedef struct {
	int		count;			 // ����� ����
	int		capacity;		// ������ �� �ִ� �ִ� ����
	tTCELL	*cells; // Ž���� �κ� ���ڿ� ����
} tCELLVECTOR;

///////////////////////////////////////////////////////////////////////////////
static int __TraverseCallback_t
(
	void	*pParam,
	int	Size,
	int	From,
	int	Length,
	int	Hash
)
{
//	int pos = Tab2Pos( Size, From, Length);		 // ASCII�� (1 ����Ʈ)
	tTCELL *cell;
	tCELLVECTOR *cellvector = (tCELLVECTOR *)pParam;

	if (From == 2) return 1; // �������� ����

	// �Ҵ�� ������ �ʰ��ϸ�
	if (cellvector->count+1 >= cellvector->capacity)
	{
		cellvector->capacity *= 2;
		cellvector->cells = (tTCELL *)realloc( (void *)(cellvector->cells),
		sizeof( tTCELL) * cellvector->capacity);
		assert( cellvector->cells != NULL);
	}
	///**/	fprintf( stderr, "from, len = %d, %d\n", From, Length);

	cell = &cellvector->cells[cellvector->count];

	cell->from = From;
	cell->len = Length;
	cell->hash = Hash;

	cellvector->count++;

	return 1;
}

////////////////////////////////////////////////////////////////////////////////
static int __FindAllSubstring
(
	void		*Fst,
	void		*Handle,
	void		*Freq,
	const char	*Str,
	int		Size,
	tCELL		*Tabular
)
{
	int num_searched;
	int i;
	tCELLVECTOR *searched_cells = NULL;
	int from;
	int len;
	int pos;
	int hash;
	int freq;
	int count;

	searched_cells = (tCELLVECTOR *)malloc( sizeof( tCELLVECTOR));

	searched_cells->cells = (tTCELL *)malloc( sizeof( tTCELL) * DEFAULT_CELLNUM);
	searched_cells->count = 0;
	searched_cells->capacity = DEFAULT_CELLNUM;

	// ��� �κ� ���ڿ� ã��
	fst_String2Tabular_uhc( Fst, Str, searched_cells, __TraverseCallback_t);

	num_searched = searched_cells->count; // ����� ��

#ifdef DEBUG
fprintf( stderr, "(line %d in %s)\n", __LINE__, __FILE__);
fprintf( stderr, "size = %d\n", Size);
#endif

	for (i = 0; i < num_searched; i++)
	{
		from = searched_cells->cells[i].from;
		len = searched_cells->cells[i].len;
		
		if (len % 2 == 1) continue; // 2 byte constraint failure

		if (from+len == Size*2+2) continue; // ���� "__" �������� ����� ���ڿ��� ����

#ifdef DEBUG
		char buf[len+1];
		strncpy( buf, &Str[from], len);
		buf[len] = 0;
#endif

		// ���ڿ� �յ��� __�� ����� from, len �ٽ� ���ϱ� (__????__)
		len /= 2;
		from /= 2;

		if (from != 0) from--;

		if (from == 0) len--; // ���� "__" ó��

		if (from + len > Size) len--; // ���� "__" ó��

		// ����
		pos = FSTmGetTabPos1( Size, from, len);

#ifdef DEBUG
		fprintf( stderr, "\t%d [%s]->", pos, buf);
#endif

		hash = searched_cells->cells[i].hash;

		count = Tabular[pos].count;

		if (count == 0)
		{
			Tabular[pos].candi = (tANAL *)malloc( sizeof( tANAL));
		}
		else
		{
			Tabular[pos].candi = (tANAL *)realloc( Tabular[pos].candi, sizeof( tANAL) * (count+1));
		}

		strcpy( Tabular[pos].candi[count].result, b2t_GetString ( Handle, hash)); // �м� ���

#ifdef DEBUG
		fprintf( stderr, "[%s]\n", Tabular[pos].candi[count].result);
#endif
		freq = bin_GetEntryInt( Freq, hash); // ��
		Tabular[pos].candi[count].freq = freq;
		Tabular[pos].total_freq += freq;
		Tabular[pos].count++;
	}

	// �޸� ����
	if (searched_cells->cells)
	{
		free( searched_cells->cells);
		searched_cells->cells = NULL;
	}
	free( searched_cells);
	searched_cells = NULL;

	return 1;
}

////////////////////////////////////////////////////////////////////////////////
static int __CompareStr
(
	const void *a,
	const void *b
)
{
	tANAL *x = ( tANAL *) a;
	tANAL *y = ( tANAL *) b;

	return strcmp( x->result, y->result);
}

////////////////////////////////////////////////////////////////////////////////
static int __PushResult
(
	tCELL		*Tabular,
	int		Tab,
	tANAL		*Prev,
	tANAL		*Cur
)
{
	int i;
	char result[MAX_WORD];
	tANAL *pos = Tabular[Tab].candi;
	int ret;
	double cur_prob = Prev->prob + Cur->prob; // �α� Ȯ������ ��
	double max_prob = Tabular[Tab].max_prob;

	sprintf( result, "%s%s", Prev->result, Cur->result);
//**/	fprintf( stdout, "<%s+%s>\n", Prev->result, Cur->result);
//**/	fprintf( stdout, "max = %e, cur = %e\n", max_prob, cur_prob);

	// Ȯ������ ���� cutoff
	// �ִ� Ȯ�������� ���̰� 5�� ������ skip
	// threshold�� ���ؾ� ��
	if (max_prob - cur_prob > 5) return 1;


	for (i = 0; i < Tabular[Tab].count; i++)
	{
//**/		fprintf( stdout, "prob = %lf\n", Tabular[Tab].candi[i].prob);
//**/		fprintf( stdout, "%s vs %s\n", Tabular[Tab].candi[i].result, result);

		ret = strcmp( pos[i].result, result);

		// �̹� �����ϴ� ����̸�
		if (ret == 0)
		{
			// �� Ȯ������ �������� ũ��
			if ( pos[i].prob < cur_prob)
			{
				pos[i].prob = cur_prob;

				// �ִ� Ȯ�� ����
				if (max_prob < cur_prob)
				{
					Tabular[Tab].max_prob = cur_prob;
				}
			}
			return 1; // ����� ���� ������ �ʿ����
		}
		else if (ret > 0) 
		{
//			/**/fprintf( stdout, "skipped\n");
			break; // ���ĵǾ� �����Ƿ� ���̻� �� �ʿ����
		}
	}
 
	// �޸� �Ҵ� 
	if (Tabular[Tab].count == 0)
	{
		Tabular[Tab].candi = (tANAL *)malloc( sizeof( tANAL));
	}
	else
	{
		Tabular[Tab].candi = (tANAL *)realloc( Tabular[Tab].candi, sizeof( tANAL) * (Tabular[Tab].count+1));
	}
 
	// ����
	strcpy( Tabular[Tab].candi[Tabular[Tab].count].result, result);
	Tabular[Tab].candi[Tabular[Tab].count].prob = cur_prob;
	Tabular[Tab].count++;

	// �ִ� Ȯ�� ����
	if (max_prob < cur_prob)
	{
		Tabular[Tab].max_prob = cur_prob;
	}

	// sorting
	qsort( Tabular[Tab].candi, Tabular[Tab].count, sizeof( tANAL), __CompareStr);

	return 1;
}

////////////////////////////////////////////////////////////////////////////////
static int __CompareProb
(
	const void *a,
	const void *b
)
{
	tANAL *x = ( tANAL *) a;
	tANAL *y = ( tANAL *) b;
	
	if (x->prob < y->prob) return 1;
	else return -1;
}

////////////////////////////////////////////////////////////////////////////////
static int __CYK_Run
(
	tCELL			*Tabular,
	const char		*Str,
	int			Size,
	int			NumCell
)
{
	int i;
	int j;
	int prev, cur;

	int cur_tab; // ����
	int front_tab; // ����
	int total_tab; // ���� + ����
	double max_prob;
	double cur_prob;

	// Ȯ�� ��� 
	for (i = 0; i < NumCell; i++)
	{
		max_prob = Tabular[i].max_prob; // �ʱ�ȭ

		if (Tabular[i].count > 0)
		{
			for (j = 0; j < Tabular[i].count; j++)
			{
				cur_prob = Tabular[i].candi[j].prob = log( (double) Tabular[i].candi[j].freq / Tabular[i].total_freq);

				if (max_prob < cur_prob)
				{
					Tabular[i].max_prob = max_prob = cur_prob;
				}
			}
		}
	}

	for (j = 2; j <= Size; j++)
	{
	 
		total_tab = TabPos2to1( 0, j, Size); // ���� + ����
 
		// ��ü(���� + ����)�� ����Ǿ� ������ skip
		// �̹� �� �κ��� �����Ƿ� �� �κ��� ������ �ǹ̾��ٰ� �Ǵ�
		if ( Tabular[total_tab].count > 0) continue;

		for (i = j-1; i >= 1; i--)
		{
	
			// T(0, i) : ����
			// T(i, j) : ����
			// T(0, j) : ��ģ ���

			cur_tab = TabPos2to1( i, j, Size); // ����
			front_tab = TabPos2to1( 0, i, Size); // ����
	
			// �պκ� + ����κ�
			// ���� �κ��� ������ ��ϵǾ� ���� ������
			if ( Tabular[cur_tab].count == 0) continue;

			// �պκ��� ����� ������
			if ( Tabular[front_tab].count == 0) continue;

			// �պκ��� ��� ����� ����
			for ( prev = 0; prev < Tabular[front_tab].count; prev++)
			{
				// ����κп� ����
				for ( cur = 0; cur < Tabular[cur_tab].count; cur++)
				{
					__PushResult( Tabular, total_tab, &Tabular[front_tab].candi[prev], &Tabular[cur_tab].candi[cur]);
#ifdef DEBUG
					fprintf( stderr, "%d T(0,%d)=%d T(0,%d) + %d T(%d,%d) %s+%s\n", total_tab, j, front_tab, i, cur_tab, i, j, Tabular[front_tab].candi[prev].result, Tabular[cur_tab].candi[cur].result);
#endif
				}
			}
		}
	}

	// ���� ��� sorting
	qsort( Tabular[Size-1].candi, Tabular[Size-1].count, sizeof( tANAL), __CompareProb);

	return 1; 
}

////////////////////////////////////////////////////////////////////////////////
int phonetic_Recover
(
	PHONETICtRSC	*Rsc,
	const char		*Str,			// [input] �Է� ����
	MAtRESULTS		*RestoredStr	// [output] ���� ���� ���
)
{
	char ej_by_2byte[MAX_WORD]; // 2����Ʈ(����) ������ ���� 
	char ej_by_2byte_ht[MAX_WORD]; // __����__
	int len;
	int cellnum;
	int i;
	tCELL *tabular;

	if (Str == NULL || Str[0] == 0) return 0;

#ifdef DEBUG
	fprintf( stderr, "������ ���� : %s\n", Str);
#endif

	// FIL�� �߰��Ͽ� �� ������ 2byte�� �����.
	split_by_char_array( (char *)Str, ej_by_2byte);

	// �յڿ� "__" �߰�
	sprintf( ej_by_2byte_ht, "__%s__", ej_by_2byte);

#ifdef DEBUG
	fprintf( stderr, "������ ����(2b) : %s\n", ej_by_2byte_ht);
	fprintf( stderr, "len = %d\n", strlen( ej_by_2byte));
#endif
	int len2b = strlen( ej_by_2byte);
	if (len2b % 2 == 1)
	{
		fprintf( stderr, "Error : an incomplete char occured in [%s].\n", Str);
		return 0;
	}

	len = len2b / 2; // ���ڿ� ����

#ifdef DEBUG
	fprintf( stderr, "len = %d\n", len);
#endif
	cellnum = TabNum( len);
	
	// �ʹ� �� ���� ó��
	if (len > 15)
	{
#ifdef DEBUG
	fprintf( stderr, "�ʹ� �� ���� (�������� ��ü) : %s\n", ej_by_2byte_ht);
#endif
		RestoredStr->mresult[RestoredStr->count].str = strdup( ej_by_2byte);
		RestoredStr->mresult[RestoredStr->count].prob = 0.0; // log(1)
		RestoredStr->count++;
		return 1;
	}

	tabular = (tCELL *)malloc( sizeof( tCELL) * cellnum);
	assert( tabular != NULL);

	// �ʱ�ȭ
	for (i = 0; i < cellnum; i++)
	{
		tabular[i].count = 0;
		tabular[i].total_freq = 0;
		tabular[i].max_prob = PROBcLOG_ALMOST_ZERO;
	}

	// ���� ���� ������ �ִ� ��� �κ� ���ڿ��� ã�´�.
	__FindAllSubstring( Rsc->fst, Rsc->info, Rsc->freq, ej_by_2byte_ht, len, tabular);

	// ���� ���� (CYK �˰���)
	__CYK_Run( tabular, ej_by_2byte_ht, len, cellnum);

	// ��� ���
	double log_max_prob = tabular[len-1].max_prob;
	
	for (i = 0; i < 10 /* ���� ���� ������ �ִ� �� */ && i < tabular[len-1].count; i++)
	{
		// Ȯ�� cutoff
		if (log_max_prob - tabular[len-1].candi[i].prob > 10)
		{
#ifdef DEBUG
			fprintf( stderr, "cutoffed!!\t(line %d in %s)\n", __LINE__, __FILE__);
			fprintf( stderr, "%s\t%f\n", tabular[len-1].candi[i].result, tabular[len-1].candi[i].prob);
#endif
			break;
		}

#ifdef DEBUG
		fprintf( stderr, "result : %s\t%e\n", tabular[len-1].candi[i].result, tabular[len-1].candi[i].prob);
#endif

		char *res = strdup( &tabular[len-1].candi[i].result[2]);
		res[strlen( res)-2] = 0;
		
		RestoredStr->mresult[RestoredStr->count].str = res;
		RestoredStr->mresult[RestoredStr->count].prob = tabular[len-1].candi[i].prob;
		RestoredStr->count++;
	}

	for (i = 0; i < cellnum; i++)
	{
		if (tabular[i].count > 0)
		{
			free( tabular[i].candi);
		}
	}

	free( tabular);
	tabular = NULL;

	// ���� ���� ����� ������
	if (RestoredStr->count == 0)
	{
		RestoredStr->mresult[RestoredStr->count].str = strdup( ej_by_2byte);
		RestoredStr->mresult[RestoredStr->count].prob = 0.0; // log(1)
		RestoredStr->count++;
		return 1;
	}

	return RestoredStr->count;
}

#ifdef MAIN
////////////////////////////////////////////////////////////////////////////////
int phonetic_RecoverFile
(
	PHONETICtRSC	*Rsc,
	const char		*FileName
)
{
	FILE *fp;
	char str[1024];

	fp = fopen( FileName, "rt");
	if (fp == NULL)
	{
		fprintf( stderr, "Error: cannot open file! (%s)\n", FileName);
		return -1;
	}

	while( fgets( str, 1024, fp) != NULL)
	{
		if (str[0] == 0 || str[0] == '\n') continue;
		str[strlen( str)-1] = 0;

		phonetic_Recover( Rsc, str);
	}

	fclose( fp);
	return 1;
}


////////////////////////////////////////////////////////////////////////////////
int main( int argc, char **argv)
{
	char fst_filename[100];
	char info_filename[100];
	char freq_filename[100];

	void *fst;
	int info_handle;
	void *freq;
	
	PHONETICtRSC *rsc;

	if (argc != 3)
	{
		fprintf( stderr, "%s FILE-stem in-file\n", argv[0]);
		
		return 1;
	}

	char ext_excluded_filename[100];

	{ /* Ȯ���� ���� */
		char *p;
		strcpy( ext_excluded_filename, argv[1]);
		p = strrchr( ext_excluded_filename, '.');
		if ( p != NULL) *p = 0;
	}
 
	sprintf( fst_filename,	"%s.fst", ext_excluded_filename);	 /* fst */
	sprintf( info_filename, "%s.info", ext_excluded_filename); /* info */
	sprintf( freq_filename, "%s.freq", ext_excluded_filename); /* freq */

	rsc = phonetic_Open( fst_filename, info_filename, freq_filename);
	assert( rsc != NULL);

	fprintf( stderr, "%s ..\n", argv[2]);
	phonetic_RecoverFile( fst, info_handle, freq, argv[2]);

	phonetic_Close( rsc);

	return 0;
}

#endif
