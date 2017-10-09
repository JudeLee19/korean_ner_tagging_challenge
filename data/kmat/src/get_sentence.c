#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "kmat.h"
#include "kmat_def.h"

// 마지막 함수는 stdin을 입력으로 할 수 없음

#define MAX_MJ 16384

///////////////////////////////////////////////////////////////////////////////
void FreeSentence
(
	char	**Words,
	int	NumWord
)
{
	int i;
	
	for (i = 0; i < NumWord; i++)
	{
		free( Words[i]);
	}
}

///////////////////////////////////////////////////////////////////////////////
int GetSentence_Str
(
	char	*Str,
	char	**Words
)
{
	int num_word = 0;
	char *pch;

	pch = strtok( Str, " \t\r\n");
		
	while (pch != NULL) 
	{
		// 어절 길이가 너무 긴 경우 (나머지 부분은 무시)
		if (num_word+1 >= KMATcMAX_EJ_NUM)
		{
			fprintf( stderr, "Too many words in a line! (The rest of the words will be ignored!)\n");
			return num_word;
		}

		Words[num_word++] = strdup( pch); // 문자열 복사

		pch = strtok( NULL, " \t\r\n");
	}
	
	return num_word;
}

///////////////////////////////////////////////////////////////////////////////
// fp로부터 문자열을 읽어들여 word에 저장한다.
// Words[0]부터 Words[리턴값-1]에 채워짐
// 내부에서 메모리를 할당하므로 외부에서 반드시 free해야 함
// 한줄에 한 문장
// [리턴] 문장내 단어의 수
int GetSentence_Row
(
	FILE	*Fp,
	char	**Words
)
{
	int num_word = 0;
	static char str[MAX_MJ];
	char *pch;

	// 한 줄 읽기
	while (fgets (str, MAX_MJ, Fp) != NULL) 
	{ 
		if (str[0] == '\n' || str[0] == '\r') continue; // 아무것도 없으면 다음 문장 읽기
		
		pch = strtok (str, " \t\r\n");
		
		while (pch != NULL) 
		{
			// 어절 길이가 너무 긴 경우 (나머지 부분은 무시)
			if (num_word+1 >= KMATcMAX_EJ_NUM)
			{
				fprintf( stderr, "Too many words in a line! (The rest of the words will be ignored!)\n");
				return num_word;
			}

			Words[num_word++] = strdup( pch); // 문자열 복사

			pch = strtok(NULL, " \t\r\n");
		}
		
		if (num_word == 0) continue; // 다음 문장 시도
		else 
		{
			return num_word;
		}
	}
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// 문장을 읽어옴
// Words[1]부터 Words[num_word]에 저장됨
// 한 줄에 한 단어
// 빈 줄은 문장 구분에 쓰임
int GetSentence_Column
(
	FILE	*Fp,
	char	**Words
)
{
	char line[MAX_MJ];
	int num_word = 0;

	while (fgets( line, MAX_MJ, Fp) != NULL) 
	{
		// 어절 길이가 너무 긴 경우 (나머지 부분은 무시)
		if (num_word+1 >= KMATcMAX_EJ_NUM)
		{
			fprintf( stderr, "Too many words in a line! (The rest of the words will be ignored!)\n");
			return num_word;
		}

		if (line[0] == '\n') // 문장의 끝
		{
			return num_word;
		}
		else 
		{
			line[strlen(line)-1] = 0;
			Words[num_word++] = strdup( line); // 문자열 복사
		}
	}
	return num_word;
}

///////////////////////////////////////////////////////////////////////////////
// return value : 문장의 단어수
// Words[0]부터 채워짐
// morph_analyzed_result[1]부터 채워짐
int GetSentence_MA
(
	FILE		*Fp,
	char		**Words, 
	MAtRESULTS	*Result
)
{
	static int line_num = 0;

	int num_word = 0;
	char line[MAX_MJ];

	char morph_result[MAX_MJ];
	double prob;

	char one_word[1024];
	int count = 0;

	// 문장 시작
	// 초기화
	Result->count = 1;
	Result->mresult[0].str = strdup( BOW_TAG_1);
	Result->mresult[0].prob = 1.0;

	while (fgets( line, MAX_MJ, Fp) != NULL) 
	{
		line_num++;

		if (line[0] == '\n') // 문장의 끝
		{
			if (num_word) return num_word;
		}

		if (line[0] != '\t') // 원어절 시작이면
		{
			num_word++; // 어절 수 선(pre) 증가
			count = 0;

			// 원어절, 형태소분석결과, 확률
			sscanf( line, "%s%s%lf", one_word, morph_result, &prob);
			
			Words[num_word-1] = strdup( one_word);
		}
		else 
		{
			// 형태소분석결과, 확률
			sscanf( line, "%s%lf", morph_result, &prob);
		}

		// 확률 + 분석결과 저장
		Result[num_word].mresult[count].str = strdup( morph_result);
		Result[num_word].mresult[count].prob = prob;
		Result[num_word].count = ++count;
		
		
	} // end of while
	
	return num_word;
}

///////////////////////////////////////////////////////////////////////////////
// return value : 문장의 단어수
// Words[0]부터 채워짐
// morph_analyzed_result[1]부터 채워짐
//int get_sentence_from_morphological_analyzed_text_with_sbd
//(
//	void *sb_fst,
//	FILE *Fp,
//	char **Words, 
//	MAtRESULTS *Result
//)
//{
//	static int line_num = 0;
//
//	int num_word = 0;
//	char line[MAX_MJ];
//
//	char morph_result[MAX_MJ];
//	double prob;
//
//	int quit = 0;
//
//	char one_word[1024];
//	int count = 0;
//
//
//	// 문장 시작
//	// 초기화
//	Result->count = 1;
//	Result->mresult[0].str = strdup( BOW_TAG_1);
//	Result->mresult[0].prob = 1.0;
//			
//	long curfpos = ftell(Fp); // 파일 위치
//
//	while (fgets(line, MAX_MJ, Fp) != NULL) 
//	{
//		line_num++;
//
//		if (line[0] != '\t') // 원어절 시작이면
//		{
//			if (quit) // 문장 끝
//			{
//				fseek( Fp, curfpos, SEEK_SET); // 파일 위치 되돌림
//				line_num--;
//				break;
//			}
//
//			num_word++;
//			count = 0;
//
//			// 원어절, 형태소분석결과, 확률
//			sscanf( line, "%s%s%lf", one_word, morph_result, &prob);
//
//			Words[num_word-1] = strdup( one_word);
//
//			// 문장 분리 위치이면
//			if ( sbd_is_sentence_boundary( sb_fst, one_word) == 1) // 있으면
//			{
//				quit = 1; // 문장끝 어절임
//				//**/fprintf(stdout, "문장끝\n");
//			}
//		}
//
//		else 
//		{
//			// 형태소분석결과, 확률
//			sscanf( line, "%s%lf", morph_result, &prob);
//		}
//
//		// 확률 + 분석결과 저장
//		Result[num_word].mresult[count].str = strdup( morph_result);
//		Result[num_word].mresult[count].prob = prob;
//		Result[num_word].count = ++count;
//			
//		curfpos = ftell( Fp);	// 파일 위치
//
//	} // end of while
//	
//	return num_word;
//}
