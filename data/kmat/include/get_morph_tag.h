#ifndef __get_morph_tag_H__
#define __get_morph_tag_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "strvector.h"

extern int GetMorphTag
(
	char		*PosTaggedStr,	// [input] ǰ�� ������ ������ �Է� ����
	char		Delimiter,		// [input] ������
	SVtSTRVCT	*Morphs,		// [output] ���¼� ��
	SVtSTRVCT	*Tags			// [output] ǰ���±� ��
);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
