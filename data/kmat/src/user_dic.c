#include <stdio.h>
#include <string.h>

#include "get_morph_tag.h"
#include "strvector.h"
#include "fstdic.h"

////////////////////////////////////////////////////////////////////////////////
char *Postprocessing_by_userdic
(
	const char	*Result
)
{
	int morph_num = 0; // ���� ���� ���¼� ��
	int i;
	SVtSTRVCT *morphs;
	SVtSTRVCT *tags;
	
	morphs = sv_New();
	tags = sv_New();

	// ���¼�/ǰ�� �и�
	morph_num = GetMorphTag( Result, '/', morphs, tags);

	// ���¼Ҹ���
	for (i = 0; i < morph_num; i++) 
	{
		if (i) 
		{
			fprintf( stdout, "+%s/%s", morphs->strs[i], tags->strs[i]); 
		}
		else 
		{
			fprintf( stdout, "%s/%s", morphs->strs[i], tags->strs[i]);
		}
	}
	fprintf( stdout, "\n");

	sv_Free( morphs);
	sv_Free( tags);
}
