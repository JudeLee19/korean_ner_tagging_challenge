#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "hsplit.h"
#include "get_morph_tag.h"
#include "strvector.h"

///////////////////////////////////////////////////////////////////////////////
// 어절 내의 형태소와 품사 태그를 알아낸다.
// 리턴값 : 형태소의 수
int GetMorphTag
(
	char		*PosTaggedStr,	// [input] 품사 부착된 형식의 입력 어절
	char		Delimiter,		// [input] 구분자
	SVtSTRVCT	*Morphs,		// [output] 형태소 열
	SVtSTRVCT	*Tags			// [output] 품사태그 열
)
{
	int num_morph = 0; // 형태소의 수
	char cur_char;
	int len; // 문자열의 길이
	int cur;

	char tag_sub[MAX_WORD];
	int tag_i = 0;
	
	char morph_sub[MAX_WORD];
	int morph_i = 0;

	// 초기화
	len = (int) strlen( PosTaggedStr); // 문자열의 길이
	
	// 분석 불능이면
	if (strncmp( &PosTaggedStr[len-3], "/??", 3) == 0)
	{
		strncpy( morph_sub, PosTaggedStr, len-3);
		morph_sub[len-3] = 0;
		
		strcpy( tag_sub, "??");

		sv_Push( Morphs, morph_sub); // 형태소 저장
		sv_Push( Tags, tag_sub); // 품사 태그 저장
		return 1;
	}

	for (cur = 0; cur < len; cur++)
	{
		cur_char = PosTaggedStr[cur]; // 현재 글자
		morph_sub[morph_i++] = cur_char;

		if (cur_char == Delimiter) // delimiter이면
		{
			morph_sub[--morph_i] = 0; //(char) NULL;
			cur_char = PosTaggedStr[++cur];
			
			if (cur_char == Delimiter) // delimiter이면
			{
				morph_sub[morph_i++] = cur_char;
				morph_sub[morph_i] = 0;
				cur_char = PosTaggedStr[++cur];
			}
			
			// 초기화
			tag_i = 0;
			morph_i = 0;
			
			while( cur_char != 0 && cur_char != '+' 
						 && cur_char != '\n' && cur_char != ' ' 
						 && cur_char != '\t' && cur_char != ';') 
			{
				tag_sub[tag_i++] = cur_char;
				cur++;
				cur_char = PosTaggedStr[cur];
			}
			tag_sub[tag_i] = 0;

			sv_Push( Morphs, morph_sub); // 형태소 저장
			sv_Push( Tags, tag_sub); // 품사 태그 저장
			
			num_morph++; // 형태소 수 증가
		} // end of if
	} // end of for
	
	return num_morph;
}
