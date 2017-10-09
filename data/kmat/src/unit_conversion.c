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
// 결과 생성 (음절 단위로 태깅된 것을 형태소 단위 결과로 변환)
// 리턴값 : 1 = 성공, 0 = 오류
// 주의 : S와 E에 대해서는 아직 검증하지 않았음
int ConvertSyll2Morph
(
	char	Syllalbles[][3],	// [input] 음절열
	char	*SyllTags[],	// [input] 음절태그열
	int	NumSyl,		// [input] 음절 수
	char	Delimiter,
	char	*Result		// [output] 형태소 단위 분석 결과
)
{
	int i;
	char morph[MAX_WORD] = {0,}; // 형태소
	char tag[MAX_WORD] = {0,};	 // 품사
	char tag_head;
	
	SVtSTRVCT *morph_seq; // 형태소 열
	SVtSTRVCT *tag_seq; // 태그 열

	int start_time = 2;
	int end_time = NumSyl+1;

	int tag_style = STYLE_ERROR;
	
	morph_seq = sv_New();
	tag_seq = sv_New();

	// 초기화
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

	// B나 E중에 아무것도 없을 경우
	if (tag_style == STYLE_ERROR) 
	{
		return 0;
	}

	int clear = 1; // 앞에 출력하지 못한 내용이 남아 있는가 여부

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
				
					// 초기화
					morph[0] = 0;
					tag[0] = 0;
					
					clear = 1;
				}
			}

			// 음절
			if (Syllalbles[i][0] == FIL) strcat( morph, &Syllalbles[i][1]);
			else strcat( morph, Syllalbles[i]);

			// 태그
			strcpy( tag, &SyllTags[i][2]);

			// 저장			
			sv_Push( morph_seq, morph);
			sv_Push( tag_seq, tag);
			
			// 초기화
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

			// 음절
			if (Syllalbles[i][0] == FIL) strcat( morph, &Syllalbles[i][1]);
			else strcat( morph, Syllalbles[i]);

			// 태그
			strcpy( tag, &SyllTags[i][2]);
			
			clear = 0;
			
			break;
		
		case 'I' :

			// 에러 체크
			if (tag_style == B_STYLE) 
			{
				if ( clear) // 처음에 I태그가 나온 경우
				{
					return 0;
				}
				
				// 앞 태그와 다른 경우 예) B-nc I-ef, I-nc I-ef
				if (strcmp( tag, &SyllTags[i][2]) != 0) 
				{ 
					return 0;
				}
			}

			// 에러 체크
			if (tag_style == E_STYLE)
			{
				// 맨 마지막에 I태그가 나온 경우
				if (i == end_time)
				{
					return 0;
				}

				// 맨 처음 태그가 아니고, (앞 태그가 I태그이고, 앞 태그와 다른 경우)
				if ( !clear && i > 0 && SyllTags[i-1][0] == 'I' 
						 && strcmp( &SyllTags[i-1][2], &SyllTags[i][2]) != 0)
				{
					return 0; // 에러
				}
			}

			// 형태소 저장
			// 태그는 저장할 필요없음 (앞의 것과 동일하므로)
			if (Syllalbles[i][0] == FIL) strcat( morph, &Syllalbles[i][1]); 
			else strcat( morph, Syllalbles[i]);
			
			clear = 0;
			
			break;

		case 'E' :
		
			// 음절
			if (Syllalbles[i][0] == FIL) strcat( morph, &Syllalbles[i][1]);
			else strcat( morph, Syllalbles[i]);

			// 태그
			strcpy( tag, &SyllTags[i][2]);
			
			sv_Push( morph_seq, morph);
			sv_Push( tag_seq, tag);
			
			morph[0] = 0;
			tag[0] = 0;
			
			clear = 1;
			
			break;
		} // end of switch
	} // end of for
	
	// 마무리
	if (tag_style == B_STYLE && !clear) // B_STYLE의 경우 마지막 음절 태그는 loop안에서 다 출력되지 않음
	{
		// 저장
		sv_Push( morph_seq, morph);
		sv_Push( tag_seq, tag);
		
		morph[0] = 0;
		tag[0] = 0;
	}

	// 출력
	{
		if (strlen( tag_seq->strs[0]) == 0) 
		{
			///**/fprintf(stderr, "빈태그발견\n");
			return 0; // 빈 태그이면 에러 발생
		}

		sprintf( Result, "%s%c%s", morph_seq->strs[0], Delimiter, tag_seq->strs[0]); // 첫번째 형태소와 태그

		for (i = 1; i < morph_seq->count; i++) 
		{
			if ( strlen( tag_seq->strs[i]) == 0) 
			{
				///**/fprintf(stderr, "빈태그발견\n");
				return 0; // 빈 태그이면 에러 발생
			}
			sprintf( Result+strlen(Result), "+%s%c%s", morph_seq->strs[i], Delimiter, tag_seq->strs[i]);
		}
	}
	
	sv_Free( morph_seq);
	sv_Free( tag_seq);
	
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// 생성된 태그열의 정당성 검사
// tag_sequence는 음절 태그로 되어 있다. 예) B-nc
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
//		// 찾아본다.
//		num = (int) count( syl_tag_seq[i].begin(),
//					syl_tag_seq[i].end(),
//					SyllTags[i]);
//
//		if ( !num) return 0; // 결합 불가
//	}
//		
//	return 1; // 결합 가능
//}

///////////////////////////////////////////////////////////////////////////////
// 어휘층 어절의 각 음절에 대응되는 태그를 결정
// 결과는 syllable_tags에 저장
//static void get_syllable_tagging_result
//(
//	int	Mode,		// [input] 출력 모드
//	int	NumChar, 	// [input] 음절 수
//	char	*Tag,		// [input] 품사
//	char	SyllableTags[][MAX_TAG_LEN]	// [output] 음절 태그열
//)
//{
//	int j;
//	
//	for (j = 0; j < NumChar; j++) 
//	{
//		if ((Mode == BIS || Mode == IES) && NumChar == 1)
//		{
//			sprintf( SyllableTags[j], "S-%s", Tag); // 단독
//			return;
//		}
//		
//		// 시작
//		if ((Mode == BI || Mode == BIS) && !j) 
//		{
//			sprintf( SyllableTags[j], "B-%s", Tag);
//		}
//		
//		// 끝
//		else if ((Mode == IE || Mode == IES) && j == NumChar-1)
//		{
//			sprintf( SyllableTags[j], "E-%s", Tag); 
//		}
//
//		else
//		{
//			sprintf( SyllableTags[j], "I-%s", Tag); // 중간
//		}
//	} // end of for
//}

///////////////////////////////////////////////////////////////////////////////
// str : 형태소 분석 결과
// 리턴값 : 어절내 형태소의 수
//int check_morpheme_result
//(
//	char *str,
//	RESTORED_STAGS &str_syl_tag_seq,
//	char Delimiter
//)
//{
//	int morph_num = 0; // 어절 내의 형태소 수
//	SVtSTRVCT *morphs; // 형태소 열
//	SVtSTRVCT *tags; // 태그 열
//	int lexical_ej_len = 0; // 어휘층 어절 길이
//	char syllable_tags[MAX_WORD][MAX_TAG_LEN]; // 표층 어절에 대한 음절 단위의 품사 태그
//	int num_char = 0;
//	int i;
//
//	morphs = sv_New();
//	tags = sv_New();
//	
//	// 어절 내의 형태소와 품사 태그를 알아낸다.
//	morph_num = GetMorphTag( str, Delimiter, morphs, tags);
//
//	if ( !morph_num) return 0;
//
//	// 어휘층 어절 및 음절 태그 구하기
//	char lexical_ej_str[MAX_WORD] = {0,};
//	
//		
//	for (i = 0; i < morph_num; i++) // 각 형태소에 대해
//	{
//		num_char = strlen( morphs->strs[i]) / 2;
//		strcat( lexical_ej_str, morphs->strs[i]); // 어휘층 어절 구하기
//
//		// 음절 단위 태깅
//		get_syllable_tagging_result( BI, num_char, tags->strs[i], &syllable_tags[lexical_ej_len]);
//
//		lexical_ej_len += num_char; // 어휘층 어절 길이
//	}
//
//	///**/fprintf(stderr, "어휘층어절 = %s\n", lexical_ej_str);
//	if ( !check_tag_sequence( syllable_tags, str_syl_tag_seq[lexical_ej_str])) 
//	{
//		///**/fprintf(stderr, "불가능한 조합 str = %s\n", str);
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
// 음절 태그를 하나씩 얻어냄
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
