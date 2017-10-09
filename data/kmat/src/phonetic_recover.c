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
	int	from; // 시작위치
	int	len;	// 문자열 길이
	int	hash; // hash value
} tTCELL;

typedef struct {
	int		count;			 // 저장된 개수
	int		capacity;		// 저장할 수 있는 최대 개수
	tTCELL	*cells; // 탐색된 부분 문자열 정보
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
//	int pos = Tab2Pos( Size, From, Length);		 // ASCII용 (1 바이트)
	tTCELL *cell;
	tCELLVECTOR *cellvector = (tCELLVECTOR *)pParam;

	if (From == 2) return 1; // 저장하지 않음

	// 할당된 영역을 초과하면
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

	// 모든 부분 문자열 찾기
	fst_String2Tabular_uhc( Fst, Str, searched_cells, __TraverseCallback_t);

	num_searched = searched_cells->count; // 결과의 수

#ifdef DEBUG
fprintf( stderr, "(line %d in %s)\n", __LINE__, __FILE__);
fprintf( stderr, "size = %d\n", Size);
#endif

	for (i = 0; i < num_searched; i++)
	{
		from = searched_cells->cells[i].from;
		len = searched_cells->cells[i].len;
		
		if (len % 2 == 1) continue; // 2 byte constraint failure

		if (from+len == Size*2+2) continue; // 뒤쪽 "__" 직전에서 종료된 문자열은 제거

#ifdef DEBUG
		char buf[len+1];
		strncpy( buf, &Str[from], len);
		buf[len] = 0;
#endif

		// 문자열 앞뒤의 __를 고려한 from, len 다시 정하기 (__????__)
		len /= 2;
		from /= 2;

		if (from != 0) from--;

		if (from == 0) len--; // 앞쪽 "__" 처리

		if (from + len > Size) len--; // 뒤쪽 "__" 처리

		// 저장
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

		strcpy( Tabular[pos].candi[count].result, b2t_GetString ( Handle, hash)); // 분석 결과

#ifdef DEBUG
		fprintf( stderr, "[%s]\n", Tabular[pos].candi[count].result);
#endif
		freq = bin_GetEntryInt( Freq, hash); // 빈도
		Tabular[pos].candi[count].freq = freq;
		Tabular[pos].total_freq += freq;
		Tabular[pos].count++;
	}

	// 메모리 해제
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
	double cur_prob = Prev->prob + Cur->prob; // 로그 확률값의 합
	double max_prob = Tabular[Tab].max_prob;

	sprintf( result, "%s%s", Prev->result, Cur->result);
//**/	fprintf( stdout, "<%s+%s>\n", Prev->result, Cur->result);
//**/	fprintf( stdout, "max = %e, cur = %e\n", max_prob, cur_prob);

	// 확률값에 의한 cutoff
	// 최대 확률값과의 차이가 5를 넘으면 skip
	// threshold는 정해야 함
	if (max_prob - cur_prob > 5) return 1;


	for (i = 0; i < Tabular[Tab].count; i++)
	{
//**/		fprintf( stdout, "prob = %lf\n", Tabular[Tab].candi[i].prob);
//**/		fprintf( stdout, "%s vs %s\n", Tabular[Tab].candi[i].result, result);

		ret = strcmp( pos[i].result, result);

		// 이미 존재하는 결과이면
		if (ret == 0)
		{
			// 새 확률값이 기존보다 크면
			if ( pos[i].prob < cur_prob)
			{
				pos[i].prob = cur_prob;

				// 최대 확률 갱신
				if (max_prob < cur_prob)
				{
					Tabular[Tab].max_prob = cur_prob;
				}
			}
			return 1; // 결과를 새로 저장할 필요없음
		}
		else if (ret > 0) 
		{
//			/**/fprintf( stdout, "skipped\n");
			break; // 정렬되어 있으므로 더이상 볼 필요없음
		}
	}
 
	// 메모리 할당 
	if (Tabular[Tab].count == 0)
	{
		Tabular[Tab].candi = (tANAL *)malloc( sizeof( tANAL));
	}
	else
	{
		Tabular[Tab].candi = (tANAL *)realloc( Tabular[Tab].candi, sizeof( tANAL) * (Tabular[Tab].count+1));
	}
 
	// 저장
	strcpy( Tabular[Tab].candi[Tabular[Tab].count].result, result);
	Tabular[Tab].candi[Tabular[Tab].count].prob = cur_prob;
	Tabular[Tab].count++;

	// 최대 확률 갱신
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

	int cur_tab; // 현재
	int front_tab; // 앞쪽
	int total_tab; // 앞쪽 + 현재
	double max_prob;
	double cur_prob;

	// 확률 계산 
	for (i = 0; i < NumCell; i++)
	{
		max_prob = Tabular[i].max_prob; // 초기화

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
	 
		total_tab = TabPos2to1( 0, j, Size); // 앞쪽 + 현재
 
		// 전체(앞쪽 + 현재)가 저장되어 있으면 skip
		// 이미 긴 부분이 있으므로 두 부분의 결합은 의미없다고 판단
		if ( Tabular[total_tab].count > 0) continue;

		for (i = j-1; i >= 1; i--)
		{
	
			// T(0, i) : 앞쪽
			// T(i, j) : 현재
			// T(0, j) : 합친 결과

			cur_tab = TabPos2to1( i, j, Size); // 현재
			front_tab = TabPos2to1( 0, i, Size); // 앞쪽
	
			// 앞부분 + 현재부분
			// 현재 부분이 사전에 등록되어 있지 않으면
			if ( Tabular[cur_tab].count == 0) continue;

			// 앞부분의 결과가 없으면
			if ( Tabular[front_tab].count == 0) continue;

			// 앞부분의 모든 결과에 대해
			for ( prev = 0; prev < Tabular[front_tab].count; prev++)
			{
				// 현재부분에 대해
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

	// 최종 결과 sorting
	qsort( Tabular[Size-1].candi, Tabular[Size-1].count, sizeof( tANAL), __CompareProb);

	return 1; 
}

////////////////////////////////////////////////////////////////////////////////
int phonetic_Recover
(
	PHONETICtRSC	*Rsc,
	const char		*Str,			// [input] 입력 어절
	MAtRESULTS		*RestoredStr	// [output] 음운 복원 결과
)
{
	char ej_by_2byte[MAX_WORD]; // 2바이트(음절) 단위의 어절 
	char ej_by_2byte_ht[MAX_WORD]; // __어절__
	int len;
	int cellnum;
	int i;
	tCELL *tabular;

	if (Str == NULL || Str[0] == 0) return 0;

#ifdef DEBUG
	fprintf( stderr, "복원전 어절 : %s\n", Str);
#endif

	// FIL을 추가하여 각 음절을 2byte로 만든다.
	split_by_char_array( (char *)Str, ej_by_2byte);

	// 앞뒤에 "__" 추가
	sprintf( ej_by_2byte_ht, "__%s__", ej_by_2byte);

#ifdef DEBUG
	fprintf( stderr, "복원전 어절(2b) : %s\n", ej_by_2byte_ht);
	fprintf( stderr, "len = %d\n", strlen( ej_by_2byte));
#endif
	int len2b = strlen( ej_by_2byte);
	if (len2b % 2 == 1)
	{
		fprintf( stderr, "Error : an incomplete char occured in [%s].\n", Str);
		return 0;
	}

	len = len2b / 2; // 문자열 길이

#ifdef DEBUG
	fprintf( stderr, "len = %d\n", len);
#endif
	cellnum = TabNum( len);
	
	// 너무 긴 어절 처리
	if (len > 15)
	{
#ifdef DEBUG
	fprintf( stderr, "너무 긴 어절 (원어절로 대체) : %s\n", ej_by_2byte_ht);
#endif
		RestoredStr->mresult[RestoredStr->count].str = strdup( ej_by_2byte);
		RestoredStr->mresult[RestoredStr->count].prob = 0.0; // log(1)
		RestoredStr->count++;
		return 1;
	}

	tabular = (tCELL *)malloc( sizeof( tCELL) * cellnum);
	assert( tabular != NULL);

	// 초기화
	for (i = 0; i < cellnum; i++)
	{
		tabular[i].count = 0;
		tabular[i].total_freq = 0;
		tabular[i].max_prob = PROBcLOG_ALMOST_ZERO;
	}

	// 음운 복원 정보가 있는 모든 부분 문자열을 찾는다.
	__FindAllSubstring( Rsc->fst, Rsc->info, Rsc->freq, ej_by_2byte_ht, len, tabular);

	// 음운 복원 (CYK 알고리즘)
	__CYK_Run( tabular, ej_by_2byte_ht, len, cellnum);

	// 결과 출력
	double log_max_prob = tabular[len-1].max_prob;
	
	for (i = 0; i < 10 /* 음운 복원 어절의 최대 수 */ && i < tabular[len-1].count; i++)
	{
		// 확률 cutoff
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

	// 음운 복원 결과가 없으면
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

	{ /* 확장자 제거 */
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
