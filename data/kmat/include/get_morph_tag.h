#ifndef __get_morph_tag_H__
#define __get_morph_tag_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "strvector.h"

extern int GetMorphTag
(
	char		*PosTaggedStr,	// [input] 품사 부착된 형식의 입력 어절
	char		Delimiter,		// [input] 구분자
	SVtSTRVCT	*Morphs,		// [output] 형태소 열
	SVtSTRVCT	*Tags			// [output] 품사태그 열
);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
