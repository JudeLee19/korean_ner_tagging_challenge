#include <string.h>
#include <stdio.h>

#include "phonetic_recover.h"
#include "hsplit.h" // FIL
#include "get_morph_tag.h"
#include "kmat_def.h" // MAX_TAG_LEN

#include "strvector.h"

#define B_STYLE 1
#define E_STYLE 2
#define STYLE_ERROR 3

#define BI  1
#define BIS 2
#define IE  3
#define IES 4

///////////////////////////////////////////////////////////////////////////////
// ��� ���� (���� ������ �±�� ���� ���¼� ���� ����� ��ȯ)
// ���ϰ� : 1 = ����, 0 = ����
// ���� : S�� E�� ���ؼ��� ���� �������� �ʾ���
int ConvertSyll2Morph
(
	char	Syllalbles[][3],	// [input] ������
	char	*SyllTags[],	// [input] �����±׿�
	int	NumSyl,		// [input] ���� ��
	char	Delimiter,
	char	*Result		// [output] ���¼� ���� �м� ���
)
{
	int i;
	char morph[MAX_WORD] = {0,}; // ���¼�
	char tag[MAX_WORD] = {0,};	 // ǰ��
	char tag_head;
	
	SVtSTRVCT *morph_seq; // ���¼� ��
	SVtSTRVCT *tag_seq; // �±� ��

	int start_time = 2;
	int end_time = NumSyl+1;

	int tag_style = STYLE_ERROR;
	
	morph_seq = sv_New();
	tag_seq = sv_New();

	// �ʱ�ȭ
	i = start_time;
	while (i <= end_time) 
	{
		if (SyllTags[i][0] == 'B') 
		{
			tag_style = B_STYLE;
			break;
		}
		else if (SyllTags[i][0] == 'E') 
		{
			tag_style = E_STYLE;
			break;
		}
		i++;
	}

	// B�� E�߿� �ƹ��͵� ���� ���
	if (tag_style == STYLE_ERROR) 
	{
		return 0;
	}

	int clear = 1; // �տ� ������� ���� ������ ���� �ִ°� ����

	for (i = start_time; i <= end_time; i++) 
	{
		tag_head = SyllTags[i][0];

		switch (tag_head) 
		{

		case 'S' :

			if (tag_style == B_STYLE) 
			{
				if ( !clear)
				{
					sv_Push( morph_seq, morph);
					sv_Push( tag_seq, tag);
				
					// �ʱ�ȭ
					morph[0] = 0;
					tag[0] = 0;
					
					clear = 1;
				}
			}

			// ����
			if (Syllalbles[i][0] == FIL) strcat( morph, &Syllalbles[i][1]);
			else strcat( morph, Syllalbles[i]);

			// �±�
			strcpy( tag, &SyllTags[i][2]);

			// ����			
			sv_Push( morph_seq, morph);
			sv_Push( tag_seq, tag);
			
			// �ʱ�ȭ
			morph[0] = 0;
			tag[0] = 0;

			clear = 1;

			break;

		case 'B' :

			if ( !clear)
			{
				sv_Push( morph_seq, morph);
				sv_Push( tag_seq, tag);
				
				morph[0] = 0;
				tag[0] = 0;
				
				clear = 1;
			}

			// ����
			if (Syllalbles[i][0] == FIL) strcat( morph, &Syllalbles[i][1]);
			else strcat( morph, Syllalbles[i]);

			// �±�
			strcpy( tag, &SyllTags[i][2]);
			
			clear = 0;
			
			break;
		
		case 'I' :

			// ���� üũ
			if (tag_style == B_STYLE) 
			{
				if ( clear) // ó���� I�±װ� ���� ���
				{
					return 0;
				}
				
				// �� �±׿� �ٸ� ��� ��) B-nc I-ef, I-nc I-ef
				if (strcmp( tag, &SyllTags[i][2]) != 0) 
				{ 
					return 0;
				}
			}

			// ���� üũ
			if (tag_style == E_STYLE)
			{
				// �� �������� I�±װ� ���� ���
				if (i == end_time)
				{
					return 0;
				}

				// �� ó�� �±װ� �ƴϰ�, (�� �±װ� I�±��̰�, �� �±׿� �ٸ� ���)
				if ( !clear && i > 0 && SyllTags[i-1][0] == 'I' 
						 && strcmp( &SyllTags[i-1][2], &SyllTags[i][2]) != 0)
				{
					return 0; // ����
				}
			}

			// ���¼� ����
			// �±״� ������ �ʿ���� (���� �Ͱ� �����ϹǷ�)
			if (Syllalbles[i][0] == FIL) strcat( morph, &Syllalbles[i][1]); 
			else strcat( morph, Syllalbles[i]);
			
			clear = 0;
			
			break;

		case 'E' :
		
			// ����
			if (Syllalbles[i][0] == FIL) strcat( morph, &Syllalbles[i][1]);
			else strcat( morph, Syllalbles[i]);

			// �±�
			strcpy( tag, &SyllTags[i][2]);
			
			sv_Push( morph_seq, morph);
			sv_Push( tag_seq, tag);
			
			morph[0] = 0;
			tag[0] = 0;
			
			clear = 1;
			
			break;
		} // end of switch
	} // end of for
	
	// ������
	if (tag_style == B_STYLE && !clear) // B_STYLE�� ��� ������ ���� �±״� loop�ȿ��� �� ��µ��� ����
	{
		// ����
		sv_Push( morph_seq, morph);
		sv_Push( tag_seq, tag);
		
		morph[0] = 0;
		tag[0] = 0;
	}

	// ���
	{
		if (strlen( tag_seq->strs[0]) == 0) 
		{
			///**/fprintf(stderr, "���±׹߰�\n");
			return 0; // �� �±��̸� ���� �߻�
		}

		sprintf( Result, "%s%c%s", morph_seq->strs[0], Delimiter, tag_seq->strs[0]); // ù��° ���¼ҿ� �±�

		for (i = 1; i < morph_seq->count; i++) 
		{
			if ( strlen( tag_seq->strs[i]) == 0) 
			{
				///**/fprintf(stderr, "���±׹߰�\n");
				return 0; // �� �±��̸� ���� �߻�
			}
			sprintf( Result+strlen(Result), "+%s%c%s", morph_seq->strs[i], Delimiter, tag_seq->strs[i]);
		}
	}
	
	sv_Free( morph_seq);
	sv_Free( tag_seq);
	
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// ������ �±׿��� ���缺 �˻�
// tag_sequence�� ���� �±׷� �Ǿ� �ִ�. ��) B-nc
//static int check_tag_sequence
//(
//	char SyllTags[][MAX_TAG_LEN],
//	SEQ_STAGS &syl_tag_seq
//)
//{
//	int num = 0;
//	
//	int size = (int) syl_tag_seq.size();
//
//	if ( !size) return 0;
//
//	for (int i = 0; i < size; i++) 
//	{
//		// ã�ƺ���.
//		num = (int) count( syl_tag_seq[i].begin(),
//					syl_tag_seq[i].end(),
//					SyllTags[i]);
//
//		if ( !num) return 0; // ���� �Ұ�
//	}
//		
//	return 1; // ���� ����
//}

///////////////////////////////////////////////////////////////////////////////
// ������ ������ �� ������ �����Ǵ� �±׸� ����
// ����� syllable_tags�� ����
//static void get_syllable_tagging_result
//(
//	int	Mode,		// [input] ��� ���
//	int	NumChar, 	// [input] ���� ��
//	char	*Tag,		// [input] ǰ��
//	char	SyllableTags[][MAX_TAG_LEN]	// [output] ���� �±׿�
//)
//{
//	int j;
//	
//	for (j = 0; j < NumChar; j++) 
//	{
//		if ((Mode == BIS || Mode == IES) && NumChar == 1)
//		{
//			sprintf( SyllableTags[j], "S-%s", Tag); // �ܵ�
//			return;
//		}
//		
//		// ����
//		if ((Mode == BI || Mode == BIS) && !j) 
//		{
//			sprintf( SyllableTags[j], "B-%s", Tag);
//		}
//		
//		// ��
//		else if ((Mode == IE || Mode == IES) && j == NumChar-1)
//		{
//			sprintf( SyllableTags[j], "E-%s", Tag); 
//		}
//
//		else
//		{
//			sprintf( SyllableTags[j], "I-%s", Tag); // �߰�
//		}
//	} // end of for
//}

///////////////////////////////////////////////////////////////////////////////
// str : ���¼� �м� ���
// ���ϰ� : ������ ���¼��� ��
//int check_morpheme_result
//(
//	char *str,
//	RESTORED_STAGS &str_syl_tag_seq,
//	char Delimiter
//)
//{
//	int morph_num = 0; // ���� ���� ���¼� ��
//	SVtSTRVCT *morphs; // ���¼� ��
//	SVtSTRVCT *tags; // �±� ��
//	int lexical_ej_len = 0; // ������ ���� ����
//	char syllable_tags[MAX_WORD][MAX_TAG_LEN]; // ǥ�� ������ ���� ���� ������ ǰ�� �±�
//	int num_char = 0;
//	int i;
//
//	morphs = sv_New();
//	tags = sv_New();
//	
//	// ���� ���� ���¼ҿ� ǰ�� �±׸� �˾Ƴ���.
//	morph_num = GetMorphTag( str, Delimiter, morphs, tags);
//
//	if ( !morph_num) return 0;
//
//	// ������ ���� �� ���� �±� ���ϱ�
//	char lexical_ej_str[MAX_WORD] = {0,};
//	
//		
//	for (i = 0; i < morph_num; i++) // �� ���¼ҿ� ����
//	{
//		num_char = strlen( morphs->strs[i]) / 2;
//		strcat( lexical_ej_str, morphs->strs[i]); // ������ ���� ���ϱ�
//
//		// ���� ���� �±�
//		get_syllable_tagging_result( BI, num_char, tags->strs[i], &syllable_tags[lexical_ej_len]);
//
//		lexical_ej_len += num_char; // ������ ���� ����
//	}
//
//	///**/fprintf(stderr, "���������� = %s\n", lexical_ej_str);
//	if ( !check_tag_sequence( syllable_tags, str_syl_tag_seq[lexical_ej_str])) 
//	{
//		///**/fprintf(stderr, "�Ұ����� ���� str = %s\n", str);
//		sv_Free( morphs);
//		sv_Free( tags);
//		return 0;
//	}
//
//	sv_Free( morphs);
//	sv_Free( tags);
//	
//	return morph_num;
//}

///////////////////////////////////////////////////////////////////////////////
// ���� �±׸� �ϳ��� ��
int getSyllableTags
(
	char *TagSeq,	// [input]
	char *Tags[]	// [output]
)
{
	int i = 0;
	char *ptr = TagSeq;
	
	while (1)
	{
		ptr = strchr( ptr, '\t');
		if (ptr == NULL) break;
		*ptr = 0;
		ptr++;
		Tags[i++] = ptr;
	}
	
	return i;
}
