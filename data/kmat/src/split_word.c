//#define DEBUG

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "hsplit.h"

// 긴 어절을 분해하는 함수
// 완성형 코드에만 적용됨
// return value: 부분 어절의 수
int SplitWord
(
	const char	*Eojeol,	// [input] 어절
	int		Len,		// [input] Eojeol의 길이
	char		*SubEjs[],	// [output] 부분 어절열
	int		Ctypes[]	// [output] 문자 유형
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
	
	// FIL 없이 문자 단위로 분해
	num_char = split_by_char_origin( (char *)Eojeol, splitchar);
	
	//**/fprintf( stderr, "num char = %d\n", num_char);
	
	for (i = 0; i < num_char; i++)
	{
#ifdef DEBUG
		fprintf( stderr, "char[%d]/%s/", i, splitchar[i]);
#endif
		// 문자 타입 알아내기
		ctype = UHC_GetCharType( (unsigned char *)splitchar[i]);

#ifdef DEBUG		
		fprintf( stderr, " ctype = %d (%s)\n", ctype, UHCcCTYPE[Ctypes[ctype]);
#endif
		// 이전 문자 타입과 다르면
		if (i && ctype != prev_ctype)
		{
			*b_ptr = 0;
			SubEjs[num_subej] = strdup( buff); // 부분 어절 저장
			Ctypes[num_subej++] = prev_ctype; // 문자 유형
			b_ptr = buff;
		}
		
		// buff에 문자 복사
		*b_ptr++ = splitchar[i][0];

		// 2바이트 문자의 경우
		if ((unsigned char)splitchar[i][0] > 0x80)
			*b_ptr++ = splitchar[i][1];
		
		prev_ctype = ctype;
	}
	*b_ptr = 0;
	SubEjs[num_subej] = strdup( buff); // 부분 어절 저장
	Ctypes[num_subej++] = ctype; // 문자 유형

#ifdef DEBUG	
	fprintf( stderr, "num_subej = %d\n", num_subej);
	
	for (i = 0; i < num_subej; i++)
	{
		fprintf( stderr, "SubEj[%d] %s (%s)\n", i, SubEjs[i], UHCcCTYPE[Ctypes[i]]);
	}
#endif
	return num_subej;
}
