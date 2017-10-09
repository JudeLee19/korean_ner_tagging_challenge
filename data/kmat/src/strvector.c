#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "strvector.h"

////////////////////////////////////////////////////////////////////////////////
// �Ҵ�� �޸𸮸� ����
void sv_Free
(
	SVtSTRVCT *StrVct	// [input] ���ڿ� ����
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
	int 		NumElem	// [input] ����� ��
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
	SVtSTRVCT	*StrVect,	// [input] ���ڿ� ����
	const char	*Str
)
{
	if (Str == NULL) return;
	
	if (StrVect->count == 0) // ����ִٸ�
	{
		StrVect->strs = (char **) malloc( sizeof( char *) * 5);
		assert( StrVect->strs != NULL);
		StrVect->capacity = 5; // �ʱⰪ
		
		StrVect->strs[0] = strdup( Str);
		StrVect->count++;
	}
	else
	{
		// �޸� ���Ҵ��� �ʿ��ϸ�
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
// �ʱ�ȭ
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
