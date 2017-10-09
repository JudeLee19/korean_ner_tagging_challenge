#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "strvector.h"

////////////////////////////////////////////////////////////////////////////////
// 할당된 메모리를 해제
void sv_Free
(
	SVtSTRVCT *StrVct	// [input] 문자열 벡터
)
{
	int j;
	
	if (StrVct == NULL) return;

	if ( StrVct->strs)
	{
		for (j = 0; j < StrVct->count; j++)
		{
			if (StrVct->strs[j])
			{
				free( StrVct->strs[j]);
				StrVct->strs[j] = NULL;
			}
		}
		free( StrVct->strs);
		StrVct->strs = NULL;
		StrVct->count = 0;
	}

	free( StrVct);
	StrVct = NULL;
}

////////////////////////////////////////////////////////////////////////////////
void sv_FreeArr
(
	SVtSTRVCT	*StrVctArr,	// [input]
	int 		NumElem	// [input] 결과의 수
)
{
	int i, j;
	
	if (StrVctArr == NULL) return;
	
	for (i = 0; i < NumElem; i++)
	{
		for (j = 0; j < StrVctArr[i].count; j++)
		{
			if (StrVctArr[i].strs[j])
			{
				free( StrVctArr[i].strs[j]);
				StrVctArr[i].strs[j] = NULL;
			}
		}
		free( StrVctArr[i].strs);
		StrVctArr[i].strs = NULL;
		StrVctArr[i].count = 0;
	}
	free( StrVctArr);
	StrVctArr = NULL;
}

////////////////////////////////////////////////////////////////////////////////
void sv_Push
(
	SVtSTRVCT	*StrVect,	// [input] 문자열 벡터
	const char	*Str
)
{
	if (Str == NULL) return;
	
	if (StrVect->count == 0) // 비어있다면
	{
		StrVect->strs = (char **) malloc( sizeof( char *) * 5);
		assert( StrVect->strs != NULL);
		StrVect->capacity = 5; // 초기값
		
		StrVect->strs[0] = strdup( Str);
		StrVect->count++;
	}
	else
	{
		// 메모리 재할당이 필요하면
		if (StrVect->count >= StrVect->capacity)
		{
			assert( StrVect->capacity > 0);
			StrVect->strs = (char **) realloc( StrVect->strs, sizeof( char *) * StrVect->capacity * 2);
			StrVect->capacity *= 2;
		}
		
		StrVect->strs[StrVect->count++] = strdup( Str);
	}
}

////////////////////////////////////////////////////////////////////////////////
// 초기화
SVtSTRVCT *sv_New( )
{
	SVtSTRVCT *strvct = (SVtSTRVCT *) malloc( sizeof( SVtSTRVCT));
	if (strvct == NULL) return NULL;
	
	memset( strvct, 0, sizeof( SVtSTRVCT));
	
	return strvct;
}

////////////////////////////////////////////////////////////////////////////////
SVtSTRVCT *sv_NewArr
(
	int NumElem
)
{
	SVtSTRVCT *strvctarr = (SVtSTRVCT *) malloc( sizeof( SVtSTRVCT) * NumElem);
	if (strvctarr == NULL) return NULL;
	
	memset( strvctarr, 0, sizeof( SVtSTRVCT) * NumElem);
	
	return strvctarr;
}
