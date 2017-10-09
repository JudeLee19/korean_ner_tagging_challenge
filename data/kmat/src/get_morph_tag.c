#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "hsplit.h"
#include "get_morph_tag.h"
#include "strvector.h"

///////////////////////////////////////////////////////////////////////////////
// ���� ���� ���¼ҿ� ǰ�� �±׸� �˾Ƴ���.
// ���ϰ� : ���¼��� ��
int GetMorphTag
(
	char		*PosTaggedStr,	// [input] ǰ�� ������ ������ �Է� ����
	char		Delimiter,		// [input] ������
	SVtSTRVCT	*Morphs,		// [output] ���¼� ��
	SVtSTRVCT	*Tags			// [output] ǰ���±� ��
)
{
	int num_morph = 0; // ���¼��� ��
	char cur_char;
	int len; // ���ڿ��� ����
	int cur;

	char tag_sub[MAX_WORD];
	int tag_i = 0;
	
	char morph_sub[MAX_WORD];
	int morph_i = 0;

	// �ʱ�ȭ
	len = (int) strlen( PosTaggedStr); // ���ڿ��� ����
	
	// �м� �Ҵ��̸�
	if (strncmp( &PosTaggedStr[len-3], "/??", 3) == 0)
	{
		strncpy( morph_sub, PosTaggedStr, len-3);
		morph_sub[len-3] = 0;
		
		strcpy( tag_sub, "??");

		sv_Push( Morphs, morph_sub); // ���¼� ����
		sv_Push( Tags, tag_sub); // ǰ�� �±� ����
		return 1;
	}

	for (cur = 0; cur < len; cur++)
	{
		cur_char = PosTaggedStr[cur]; // ���� ����
		morph_sub[morph_i++] = cur_char;

		if (cur_char == Delimiter) // delimiter�̸�
		{
			morph_sub[--morph_i] = 0; //(char) NULL;
			cur_char = PosTaggedStr[++cur];
			
			if (cur_char == Delimiter) // delimiter�̸�
			{
				morph_sub[morph_i++] = cur_char;
				morph_sub[morph_i] = 0;
				cur_char = PosTaggedStr[++cur];
			}
			
			// �ʱ�ȭ
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

			sv_Push( Morphs, morph_sub); // ���¼� ����
			sv_Push( Tags, tag_sub); // ǰ�� �±� ����
			
			num_morph++; // ���¼� �� ����
		} // end of if
	} // end of for
	
	return num_morph;
}
