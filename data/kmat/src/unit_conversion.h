#ifndef UNIT_CONVERSION_H
#define UNIT_CONVERSION_H

#include "phonetic_recover.h"

extern int ConvertSyll2Morph
(
	char	Syllalbles[][3],	// [input] ������
	char	*SyllTags[],	// [input] �����±׿�
	int	NumSyl,		// [input] ���� ��
	char	Delimiter,
	char	*Result		// [output] ���¼� ���� �м� ���
);

//extern int check_morpheme_result
//(
//	char *str,
//	RESTORED_STAGS &str_syl_tag_seq,
//	char delimiter
//);

// ���� �±׸� �ϳ��� ��
extern int getSyllableTags
(
	char *TagSeq,	// [input] ���� �±׿� ��) B-NNG\tI-NNG
	char *Tags[]	// [output]
);
#endif
