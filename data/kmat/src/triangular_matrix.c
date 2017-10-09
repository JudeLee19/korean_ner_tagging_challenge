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
// pos��ġ�� ���ڿ��� ���� ��
short is_empty( t_TAB pos) 
{
	if (pos.x == pos.y) return 1;
	else return 0;
}

///////////////////////////////////////////////////////////////////////////////
// �ﰢ����� ��ġ pos�� �޾� ���ڿ� str�� �ش� sub ���ڿ��� dst�� ����
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
// tabular�� �� ��ġ(������ �迭)�� �Է¹޾� (x), �ﰢ����� ��ġ(i, j)�� �Ѱ��ش�.
// n�� ���ڿ��� ���� (������ ��) = Len 
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
// �־��� ���ڿ�(SrcStr)�� ��� �κ� ���ڿ� ����
// ��) ��������� -> ��, ����, �����, �������, ���������, ��, ���, ..., ��
int GetAllSubstring
(
	char	*SrcStr,	// [input] �����ڿ�
	int	NumChars,	// [input] ���� ��
	char	**AllSubstr	// [output]
)
{
	int i, j, k;

	// �κ� ���ڿ� ����
	for (k = i = 0; i < NumChars; i++) {
		for (j = i; j < NumChars; j++, k++) {
			AllSubstr[k] = strndup( SrcStr+i*2, (j-i+1)*2);
			AllSubstr[k][(j-i+1)*2] = 0;
		}
	}
	return 1;
}

