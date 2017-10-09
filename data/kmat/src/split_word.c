//#define DEBUG

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "hsplit.h"

// �� ������ �����ϴ� �Լ�
// �ϼ��� �ڵ忡�� �����
// return value: �κ� ������ ��
int SplitWord
(
	const char	*Eojeol,	// [input] ����
	int		Len,		// [input] Eojeol�� ����
	char		*SubEjs[],	// [output] �κ� ������
	int		Ctypes[]	// [output] ���� ����
)
{
	int ctype = 0;
	int prev_ctype = -1;
	char splitchar[Len][3];
	int i;
	int num_char;

	char buff[1024];
	char *b_ptr;

	int num_subej = 0;
	
	b_ptr = buff;
	
	// FIL ���� ���� ������ ����
	num_char = split_by_char_origin( (char *)Eojeol, splitchar);
	
	//**/fprintf( stderr, "num char = %d\n", num_char);
	
	for (i = 0; i < num_char; i++)
	{
#ifdef DEBUG
		fprintf( stderr, "char[%d]/%s/", i, splitchar[i]);
#endif
		// ���� Ÿ�� �˾Ƴ���
		ctype = UHC_GetCharType( (unsigned char *)splitchar[i]);

#ifdef DEBUG		
		fprintf( stderr, " ctype = %d (%s)\n", ctype, UHCcCTYPE[Ctypes[ctype]);
#endif
		// ���� ���� Ÿ�԰� �ٸ���
		if (i && ctype != prev_ctype)
		{
			*b_ptr = 0;
			SubEjs[num_subej] = strdup( buff); // �κ� ���� ����
			Ctypes[num_subej++] = prev_ctype; // ���� ����
			b_ptr = buff;
		}
		
		// buff�� ���� ����
		*b_ptr++ = splitchar[i][0];

		// 2����Ʈ ������ ���
		if ((unsigned char)splitchar[i][0] > 0x80)
			*b_ptr++ = splitchar[i][1];
		
		prev_ctype = ctype;
	}
	*b_ptr = 0;
	SubEjs[num_subej] = strdup( buff); // �κ� ���� ����
	Ctypes[num_subej++] = ctype; // ���� ����

#ifdef DEBUG	
	fprintf( stderr, "num_subej = %d\n", num_subej);
	
	for (i = 0; i < num_subej; i++)
	{
		fprintf( stderr, "SubEj[%d] %s (%s)\n", i, SubEjs[i], UHCcCTYPE[Ctypes[i]]);
	}
#endif
	return num_subej;
}
