#include <stdio.h>
#include <stdlib.h>

#include "get_sentence.h"
#include "kmat.h"

const char *Version = __DATE__;
const char *Description = "KMAT: Korean morphological analyzer & part-of-speech tagger";

#define RSC_PATH "../rsc"

///////////////////////////////////////////////////////////////////////////////
int main (int argc, char *argv[])
{
	void *rsc = NULL; // 리소스 (형태소분석기+태거)
	int num_word = 0; // 문장내 어절 수
	char *words[KMATcMAX_EJ_NUM]; // 문장내 어절
	char *results[KMATcMAX_EJ_NUM]; // 각 어절에 대한 형태소 분석 결과 (태깅 결과)
	
	int num_line = 0;
	int i = 1;
	
	// 화일 단위 처리
	FILE *infp;		// 입력 파일 포인터
	FILE *outfp; // 출력 파일 포인터
	int is_stdin = 0; // 표준 입출력 처리 여부

	char outfile_name[200];

	/////////////////////////////////////////////////////////////////////////////
	fprintf( stderr, "\n%s (compile date: %s)\n", Description, Version); // 프로그램 정보 출력
	
	///////////////////////////////////////////////////////////////////////////////
	// 리소스 열기
	///////////////////////////////////////////////////////////////////////////////
	rsc = kmat_Open( RSC_PATH);
	if ( !rsc)
	{
		fprintf( stderr, "\n[ERROR] Initialization failure.\n");
		return 0;
	}
	fprintf( stderr, "Initialization..\t[done]\n");
	
	if (argc == 1) is_stdin = 1;
	
	while( is_stdin || i < argc)
	{
		if (is_stdin) // 표준 입출력 처리이면
		{
			infp = stdin;
			outfp = stdout;
			is_stdin = 0; // 더 이상 반복하면 안되니까
		}
		else // 파일 처리이면
		{
			if ((infp = fopen( argv[i], "rt")) == NULL) 
			{
				fprintf( stderr, "File open error : %s\n", argv[i]);
				return 0;
			}

			// 출력 파일 열기
			sprintf( outfile_name, "%s.out", argv[i]);
			if ((outfp = fopen( outfile_name, "wt")) == NULL) 
			{
				fprintf( stderr, "File open error : %s\n", outfile_name);
				return 0;
			}

			fprintf( stderr, " %s -> %s\n", argv[i], outfile_name);

			i++;
		}

		///////////////////////////////////////////////////////////////////////////////
		// 문장 입력 -> 형태소 분석 & 태깅 -> 결과 출력 -> 메모리 해제
		///////////////////////////////////////////////////////////////////////////////
		while ((num_word = GetSentence_Row( infp, words)) > 0) // 모든 문장에 대해 반복			
		{
			num_line++;
			
			if (num_line % 100000 == 0) fprintf( stderr, "%d lines..\n", num_line);
			
			////////////////////////////////////////////////////////////
			// 형태소 분석 / 태깅
			kmat_Exec( rsc, words, num_word, results);
			
			// 결과 출력
			kmat_PrintResult( outfp, words, num_word, results);
			
			// 문장 경계를 출력한다.
			fprintf( outfp, "\n");
			
			// 문자열 메모리 해제
			FreeSentence( words, num_word);	// 문장
			FreeSentence( results, num_word);	// 분석 결과
      	
		} // end of while
		
		if (! is_stdin)
		{
			// 화일 닫기
			fclose( infp);
			fclose( outfp);
		}
	}
	///////////////////////////////////////////////////////////////////////////////
	// 리소스 닫기
	///////////////////////////////////////////////////////////////////////////////
	fprintf( stderr, "Closing the POS tagger..");
	kmat_Close( rsc);
	fprintf( stderr, "\t[done]\n");

	return 0;
}
