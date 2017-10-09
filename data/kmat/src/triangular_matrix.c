#ifndef _GNU_SOURCE
#define _GNU_SOURCE // for strndup
#endif

#include <string.h>
#include <stdio.h>

#include "triangular_matrix.h"

///////////////////////////////////////////////////////////////////////////////
void setpos( t_TAB *pos, int x, int y) 
{
	pos->x = x;
	pos->y = y;
}

///////////////////////////////////////////////////////////////////////////////
// pos위치에 문자열이 없을 때
short is_empty( t_TAB pos) 
{
	if (pos.x == pos.y) return 1;
	else return 0;
}

///////////////////////////////////////////////////////////////////////////////
// 삼각행렬의 위치 pos를 받아 문자열 str의 해당 sub 문자열을 dst에 저장
char *TabPos2String
(
	char *src,
	char *dst,
	t_TAB *pos
)
{
	char *s1;

	s1 = src + (pos->x)*2;

	strncpy(dst, s1, ((pos->y) - (pos->x)) * 2);
	dst[((pos->y) - (pos->x)) * 2] = 0;

	return dst;
}

///////////////////////////////////////////////////////////////////////////////
// tabular의 셀 위치(일차원 배열)를 입력받아 (x), 삼각행렬의 위치(i, j)를 넘겨준다.
// n은 문자열의 길이 (음절의 수) = Len 
// see also TabPos2to1
short TabPos1to2( int x, int n, t_TAB *pos)
{
	int k;

	pos->x = -1;
	for (k = 0; k < n; k++) 
	{
		if ( x < n+(2*n-k-1)*k/2 ) 
		{
			pos->x = k;
			break;
		}
	}

	if ((pos->x) == -1) 
	{
		fprintf( stderr, "Error: TabPos2Pos() : out of range!\n");
		return 0;
	}
				
	pos->y = x - (n + (2*n-(pos->x)) * ((pos->x)-1) / 2) + (pos->x) + 1;

	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// 주어진 문자열(SrcStr)의 모든 부분 문자열 저장
// 예) 현재까지의 -> 현, 현재, 현재까, 현재까지, 현재까지의, 재, 재까, ..., 의
int GetAllSubstring
(
	char	*SrcStr,	// [input] 원문자열
	int	NumChars,	// [input] 음절 수
	char	**AllSubstr	// [output]
)
{
	int i, j, k;

	// 부분 문자열 저장
	for (k = i = 0; i < NumChars; i++) {
		for (j = i; j < NumChars; j++, k++) {
			AllSubstr[k] = strndup( SrcStr+i*2, (j-i+1)*2);
			AllSubstr[k][(j-i+1)*2] = 0;
		}
	}
	return 1;
}

