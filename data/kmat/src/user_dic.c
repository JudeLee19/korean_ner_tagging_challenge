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
	int morph_num = 0; // 어절 내의 형태소 수
	int i;
	SVtSTRVCT *morphs;
	SVtSTRVCT *tags;
	
	morphs = sv_New();
	tags = sv_New();

	// 형태소/품사 분리
	morph_num = GetMorphTag( Result, '/', morphs, tags);

	// 형태소마다
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
