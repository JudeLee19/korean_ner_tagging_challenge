#ifndef UNIT_CONVERSION_H
#define UNIT_CONVERSION_H

#include "phonetic_recover.h"

extern int ConvertSyll2Morph
(
	char	Syllalbles[][3],	// [input] 음절열
	char	*SyllTags[],	// [input] 음절태그열
	int	NumSyl,		// [input] 음절 수
	char	Delimiter,
	char	*Result		// [output] 형태소 단위 분석 결과
);

//extern int check_morpheme_result
//(
//	char *str,
//	RESTORED_STAGS &str_syl_tag_seq,
//	char delimiter
//);

// 음절 태그를 하나씩 얻어냄
extern int getSyllableTags
(
	char *TagSeq,	// [input] 음절 태그열 예) B-NNG\tI-NNG
	char *Tags[]	// [output]
);
#endif
