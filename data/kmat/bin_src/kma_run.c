//#define DEBUG

#include <stdio.h>
#include <stdlib.h>

#include "get_sentence.h"
#include "kmat.h"

const char *Version = __DATE__;
const char *Description = "KMA: Korean morphological analyzer";

///////////////////////////////////////////////////////////////////////////////
int main (int argc, char *argv[])
{
	void *ma_rsc = NULL; // 형태소 분석기 리소스
	MAtRESULTS *ma_results = NULL;	// 형태소 분석 결과 저장
	char *words[KMATcMAX_EJ_NUM]; // 문장내 어절
	int num_word = 0; // 문장내 어절 수
	int i;
	int ret;
	
	int num_line = 0;

	/////////////////////////////////////////////////////////////////////////////
	fprintf( stderr, "\n%s (compile date: %s)\n", Description, Version); // 프로그램 정보 출력
	
	///////////////////////////////////////////////////////////////////////////////
	// 형태소 분석 리소스 열기
	///////////////////////////////////////////////////////////////////////////////
	ma_rsc = ma_Open( "../rsc", 7); // EOJEOL_ANALYSIS | MORPHEME_ANALYSIS | SYLLABLE_ANALYSIS
	
	if ( !ma_rsc)
	{
		fprintf( stderr, "\n[ERROR] Initialization failure.\n");
		return 0;
	}
	fprintf( stderr, "Initialization..\t[done]\n");

	///////////////////////////////////////////////////////////////////////////////
	// 문장 입력 -> 형태소 분석 -> 출력 -> 메모리 해제
	///////////////////////////////////////////////////////////////////////////////
	while ((num_word = GetSentence_Row( stdin, words)) > 0) // 모든 문장에 대해 반복
	{
		num_line++;
		
		if (num_line % 100000 == 0) fprintf( stderr, "%d lines..\n", num_line);
		
		////////////////////////////////////////////////////////////
		// 메모리 할당
		ma_results = ma_New( num_word);

		// 각 어절에 대한 형태소 분석
		for (i = 0; i < num_word; i++) 
		{
			// 형태소 분석
			ret = ma_Exec( words[i], ma_rsc,
						EOJEOL_ANALYSIS|MORPHEME_ANALYSIS|SYLLABLE_ANALYSIS,
						&ma_results[i]);
		} // end of for

		// 형태소 분석 결과 출력
		for (i = 0; i < num_word; i++) 
		{
			ma_PrintResult( stdout, words[i], &ma_results[i]);
		}
		
		// 분석 결과에 대한 메모리 해제
		ma_FreeAll( ma_results, num_word);

		// 문자열 메모리 해제
		FreeSentence( words, num_word);
		
		// 문장 경계 출력
		fprintf( stdout, "\n");
	} // end of while

	///////////////////////////////////////////////////////////////////////////////
	// 형태소 분석기 리소스 닫기
	///////////////////////////////////////////////////////////////////////////////
	fprintf( stderr, "Closing the morphological analyzer..");
	ma_Close( ma_rsc, 7);
	fprintf( stderr, "\t[done]\n");

	return 0;
}
