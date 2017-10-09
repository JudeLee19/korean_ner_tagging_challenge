//#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <limits.h> // LONG_MAX
#include <assert.h>
#include <string.h>

#include "kmat_def.h" // DELIMITER
#include "ma.h"
#include "probtool.h"
#include "triangular_matrix.h"

///////////////////////////////////////////////////////////////////////////////
static char *strRstr
(
	char		*String,
	const char	*Find
)
{
	size_t stringlen, findlen;
	char *cp;

	findlen = strlen( Find);
	stringlen = strlen( String);

	if (findlen > stringlen)
		return NULL;

	for (cp = String + stringlen - findlen; cp >= String; cp--) 
	{
		if (strncmp( cp, Find, findlen) == 0)
			return cp;
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// 실제 형태소 분석 모듈
// CYK 알고리즘
int cyk_ExecM
(
	MAtRSC_M		*Rsc,			// [input] 형태소 단위 모델 리소스
	MAtRESULTS		*SubstrInfo,	// [input] 부분 문자열의 사전 정보
	int 			NumCell,		// [input] 삼각표의 셀의 수
	int			EjIndex,		// [input] 몇 번째 어절인가?
	char			**Substr,		// [input] 부분 문자열
	int			NumSyll,		// [input] 음절 수
	double		PhoneticProb,	// [input] 음운 복원 확률
	double		CutoffThreshold,	// [input] 컷오프 임계값
	double		AbsThreshold,	// [input] 절대 임계값
	MAtRESULTS		*Result		// [output] 결과 저장
)
{
	PROBtFST *rsc_trans = Rsc->transition;	// 전이 확률
	PROBtFST *rsc_morph = Rsc->morph;		// 형태소 확률
	int i, j, k, l;
	int cur_tab; // 현재
	int front_tab; // 앞쪽
	int total_tab; // 앞쪽 + 현재

	MAtRESULTS *table; // 부분 결과 저장

	double trans_prob, trans_prob2; // 전이확률
	double morph_prob; // 형태소 확률

	static double max_prob; // 최대 확률 (static 선언에 주의)
	double cur_prob; // 현재 확률
	
	double table_max[NumCell]; // table의 각 원소에서 가장 큰 확률값을 저장
	
	table = (MAtRESULTS *)malloc( sizeof( MAtRESULTS) * NumCell);
	assert( table != NULL);

	// 초기화
	for (i = 0; i < NumCell; i++)
	{
		table[i].count = 0;
		table_max[i] = -LONG_MAX;
	}
	
	// 첫번째 어절이 아니면
	// 지금까지의 최대 확률값을 보관하고 있음
	if (EjIndex == 0) max_prob = -LONG_MAX;

	/*
	영화전문
	영(0), MAG, -8.18
	영(0), NNG, -9.88
	영(0), NNP, -8.24
	영(0), NR, -8.00
	영화(1), NNG, -7.32
	화(4), NNG, -8.66
	화(4), NNP, -11.50
	화(4), XSN, -2.91
	화전(5), NNG, -10.32
	전(7), MM, -3.72
	전(7), NNB, -9.56
	전(7), NNG, -6.35
	전(7), NNP, -6.30
	전문(8), NNG, -7.65
	문(9), NNB, -10.86
	문(9), NNG, -7.83
	문(9), NNP, -7.94
	*/

	t_TAB pos;

	const char delim[2] = {DELIMITER, 0};

#ifdef DEBUG
	fprintf(stderr, "\nwelcome to new cyk_m--------\n");
	fprintf(stderr, "음운확률 = %f\n", PhoneticProb);
	
#endif

	// base case
	// 사전에 저장된 형태소들(시작 위치에 있는 형태소만)의 품사와 확률을 알아내어 테이블에 저장
	for (i = 0; i < NumSyll; i++)
	{
		double cur_max = -LONG_MAX;
		
		TabPos1to2( i, NumSyll, &pos);
			
		if (pos.x != 0) // 어절의 시작부분이 아니면
		{
			continue; // do nothing
		}
		////////////////////////////////////////////////////
		// 모든 품사에 대해
		for (j = 0; j < SubstrInfo[i].count; j++)
		{
#ifdef DEBUG
			fprintf(stderr, "%s(%d), %s, %.2lf\n", Substr[i], i, SubstrInfo[i].mresult[j].str, SubstrInfo[i].mresult[j].prob);
#endif
			// 어절 처음 태그와의 연결성 검사
			// 연결 불가능하면
			// P(첫품사 | t0)
//			if ((trans_prob = prob_GetFSTProb2( rsc_trans, SubstrInfo[i].mresult[j].str, (char *)BOW_TAG_1)) <= PROBcLOG_ALMOST_ZERO) 
//			{
//#ifdef DEBUG
//				fprintf(stderr, "불가 %s -> %s\n", BOW_TAG_1, SubstrInfo[i].mresult[j].str); //iter->first.c_str());
//#endif
//				continue;
//			}
//			
//#ifdef DEBUG
//			fprintf(stderr, "전이 확률 P(%s -> %s) = %lf\n", BOW_TAG_1, SubstrInfo[i].mresult[j].str, trans_prob);
//#endif
			// 형태소 발생 확률
			//morph_prob = prob_GetFSTProb3( rsc_morph, Substr[i], (char *)BOW_SYL_1, SubstrInfo[i].mresult[j].str); // P(m_i | m_i-1, t_i)
			morph_prob = prob_GetFSTProb2( rsc_morph, Substr[i], SubstrInfo[i].mresult[j].str); // P(m_i | t_i)
#ifdef DEBUG
			fprintf(stderr, "형태소확률 P(%s | %s, %s) = %lf\n", Substr[i], (char *)BOW_SYL_1, SubstrInfo[i].mresult[j].str, morph_prob);
#endif
			/////////////////////////////////////////////////////
			if (pos.y == NumSyll) // 어절 전체가 하나의 형태소이면
			{
				// 어절 끝 태그와의 연결성 검사
				// 연결 불가능하면
				// P(마지막품사 | 현재품사)
//				if ((trans_prob2 = prob_GetFSTProb2( rsc_trans, (char *)EOW_TAG, SubstrInfo[i].mresult[j].str)) <= PROBcLOG_ALMOST_ZERO) 
//				{
//#ifdef DEBUG
//					fprintf(stderr, "불가 %s -> %s\n", SubstrInfo[i].mresult[j].str, EOW_TAG);
//#endif
//					continue;
//				}
//
//#ifdef DEBUG
//				fprintf(stderr, "전이 확률 P(%s -> %s) = %lf\n", SubstrInfo[i].mresult[j].str, EOW_TAG, trans_prob2);
//#endif
				// 어휘 확률 + 전이확률 + 음운복원 확률 + 전이확률
				cur_prob = morph_prob + /*trans_prob +*/ PhoneticProb /*+ trans_prob2*/;
				
				// 최대 확률 갱신 (전체가 하나의 형태소이므로)
				if (cur_prob > max_prob) max_prob = cur_prob;
			}
			///////////////////////////////////////////////////////
			else // 어절의 시작부분 and 전체가 하나의 형태소가 아님
			{
				// 어휘 확률 + 전이확률 + 음운복원 확률
				cur_prob = morph_prob + /*trans_prob +*/ PhoneticProb; 
			}
			
			if (cur_prob > cur_max)
			{
				cur_max = cur_prob;
			}

			// cut-off가 가능한지 검사
			if (CutoffThreshold > 0 && max_prob - cur_prob > CutoffThreshold) 
			{
#ifdef DEBUG
				fprintf( stderr, "cutoffed\nmax_prob %lf - cur_prob %lf = %lf > threshold %lf\n\n", max_prob, cur_prob, max_prob-cur_prob, CutoffThreshold);
#endif
				continue; // 여기서 중지
			}

			// table에 저장
			// table[인덱스][형태소/품사] = 확률
			//table[it->first][Substr[it->first]+delimiter+iter->first] = cur_prob;
			char temp_result[1024];
			sprintf( temp_result, "%s%c%s", Substr[i], DELIMITER, SubstrInfo[i].mresult[j].str);
			//table[i][temp_result] = cur_prob;
			
			int count = table[i].count;
			table[i].mresult[count].str = strdup( temp_result);
			table[i].mresult[count].prob = cur_prob;
			table[i].count++;
			
#ifdef DEBUG
			fprintf(stderr, "table[%d] : %s/%s : %lf\n\n", i, Substr[i], SubstrInfo[i].mresult[j].str, cur_prob);
#endif
		} // for (j = 0; j < SubstrInfo[i].count; j++)
		
		table_max[i] = cur_max; // 최대값을 저장
		
#ifdef DEBUG
		fprintf(stderr, "table_max[%d] = %lf\n\n", i, cur_max);
#endif

		// 최대값과 확률값의 차이가 많이 나는 결과를 삭제
		if ( table[i].count > 0) // 저장된 게 있어야
		{
			// 정렬
			qsort( table[i].mresult, table[i].count, sizeof( MAtRESULT), ma_CompareProb);
		
			for (j = 1; j < table[i].count; j++) // 맨앞 확률이 가장 높을 것이므로 0은 생략
			{
				if (cur_max - table[i].mresult[j].prob > CutoffThreshold)
				{
					int k;
				
					for (k = j; k < table[i].count; k++) // 여기부터 마지막까지 삭제
					{
#ifdef DEBUG
						fprintf(stderr, "%s 삭제됨\n", table[i].mresult[k].str);
#endif
						free( table[i].mresult[k].str);
					}
					table[i].count = j;
					break;
				}
			}
		} // if ( table[i].count > 0)
	} // for (i = 0; i < NumCell; i++)

#ifdef DEBUG
	fprintf(stderr, "초기화 통과\n\n");
#endif
	//////////////////////////////////////////////////////////////////////////////
	char result[1000];
	char *prev_last_pos;
	char *prev_last_morph;
	char prev_result[1000];
	double cur_max;

	//for (j = 2; j <= NumSyll; j++) {
	//	for (i = j-1; i >= 1; i--) {

	// T(0, i) : 앞쪽
	// T(i, j) : 현재
	// T(0, j) : 합친 결과
	for (i = 1; i < NumSyll; i++) 
	{
		for (j = i+1; j <= NumSyll; j++) 
		{
			cur_tab = TabPos2to1( i, j, NumSyll); // 현재
			front_tab = TabPos2to1( 0, i, NumSyll); // 앞쪽

			// 앞부분 + 현재부분
#ifdef DEBUG
			fprintf( stderr, "\n%s + %s\n", Substr[front_tab], Substr[cur_tab]);
#endif
			// 현재부분이 사전에 등록되어 있지 않으면
			if (SubstrInfo[cur_tab].count <= 0)
			{
#ifdef DEBUG
				fprintf( stderr, "[%s]에 결과 없음\n", Substr[cur_tab]);
#endif
				continue;
			}
			
			// 앞부분의 기분석 결과가 없으면
			if (table[front_tab].count <= 0)
			{
#ifdef DEBUG
				fprintf( stderr, "[%s]에 결과 없음\n", Substr[front_tab]);
#endif
				continue;
			}

			// 앞쪽 + 현재
			total_tab = TabPos2to1( 0, j, NumSyll);
			
			// 이미 저장된 앞쪽+현재부분에 대한 최대 확률값 (이미 저장되어 있지 않다면 -LONG_MAX)
			cur_max = table_max[total_tab];
	
#ifdef DEBUG
			fprintf( stderr, "cur_max = %lf\n", cur_max);
#endif

			// 앞부분 (모든 분석 결과에 대해)
			//for (iter = it2->second.begin(); iter != it2->second.end(); ++iter)
			for (l = 0; l < table[front_tab].count; l++)
			{
				strcpy( prev_result, table[front_tab].mresult[l].str);
							
				// 이전 분석의 마지막 품사
				prev_last_pos = strRstr( prev_result, delim);
				*prev_last_pos = 0;
				prev_last_pos++;
				
				// 이전 분석의 마지막 형태소
				prev_last_morph = strRstr( prev_result, "++");
				if (prev_last_morph == NULL) 
				{
					prev_last_morph = strRstr( prev_result, "+");
					if (prev_last_morph == NULL) prev_last_morph = prev_result;
					else prev_last_morph++;
				}
				else prev_last_morph++;
				
				// 현재부분 (모든 품사에 대해)
				for (k = 0; k < SubstrInfo[cur_tab].count; k++)
				{

					// 저장할 공간이 없으면 멈춘다. (나중에 보완해야 함)
					if (table[total_tab].count >= KMATcMAX_MA_RESULT_NUM-1) break;

#ifdef DEBUG
					fprintf(stderr, "\nprev (m,t): %s, %s\n", prev_last_morph, prev_last_pos);
					fprintf(stderr, "cur	(m,t): %s, %s\n", Substr[cur_tab], SubstrInfo[cur_tab].mresult[k].str);
#endif 			 

					// 결합 가능성 검사
					// 이전 품사와의 연결성 검사
					// 전이확률 : 이전 분석의 마지막 품사 + 현재 형태소의 품사
					// P( iter2->first | prev_last_pos)
					if ((trans_prob = prob_GetFSTProb2( rsc_trans, SubstrInfo[cur_tab].mresult[k].str, prev_last_pos)) <= PROBcLOG_ALMOST_ZERO) // 연결 불가능하면 // 확인해 볼 것!
					{
#ifdef DEBUG
						fprintf(stderr, "불가 %s -> %s\n", prev_last_pos, SubstrInfo[cur_tab].mresult[k].str);
#endif
						continue;
					}
#ifdef DEBUG
					fprintf(stderr, "전이 확률 P(%s -> %s) = %lf\n", prev_last_pos, SubstrInfo[cur_tab].mresult[k].str, trans_prob);
#endif
					// 연속해서 붙을 수 없는 태그
					if (strcmp( prev_last_pos, SubstrInfo[cur_tab].mresult[k].str) == 0)
					{
						if (strcmp( prev_last_pos, "SL") == 0 
								|| strcmp(prev_last_pos, "SH") == 0 
								|| strcmp(prev_last_pos, "SN") == 0
								|| strcmp(prev_last_pos, "SE") == 0)
						{
							continue;
						}
					}

					// 형태소 확률
					morph_prob = prob_GetFSTProb3( rsc_morph, Substr[cur_tab], prev_last_morph, SubstrInfo[cur_tab].mresult[k].str); // // P(m_i | m_i-1, t_i)
#ifdef DEBUG
					fprintf(stderr, "형태소확률 P(%s | %s, %s) = %lf\n", Substr[cur_tab], prev_last_morph, SubstrInfo[cur_tab].mresult[k].str, morph_prob);
#endif 							 
					
					// 여기까지 온 경우는 연결가능한 경우임
					// 분석결과 및 확률 저장
					// 이전 분석 결과 + 현재 형태소/품사
					sprintf( result, "%s+%s%c%s", table[front_tab].mresult[l].str, Substr[cur_tab], DELIMITER, SubstrInfo[cur_tab].mresult[k].str);
					
					// 어절 끝인가?
//					if (j == NumSyll) 
//					{
//						// 어절끝 품사와의 연결성 검사
//						// P( EOW_TAG | iter2->first)
//						if ((trans_prob2 = prob_GetFSTProb2( rsc_trans, (char *)EOW_TAG, SubstrInfo[cur_tab].mresult[k].str)) <= PROBcLOG_ALMOST_ZERO) 
//						{
//#ifdef DEBUG
//							fprintf(stderr, "불가 %s -> %s\n", SubstrInfo[cur_tab].mresult[k].str, EOW_TAG);
//#endif
//							continue;
//						}
//#ifdef DEBUG
//						fprintf(stderr, "전이 확률 P(%s -> %s) = %lf\n", SubstrInfo[cur_tab].mresult[k].str, EOW_TAG, trans_prob2);
//#endif
//						// 어절끝 형태소 확률은 도움 안됨
//							
//						// 이전 분석 확률 + 전이 확률 + 어휘 확률 + (마지막 품사와 EOW간의) 전이 확률 
//						cur_prob = table[front_tab].mresult[l].prob + trans_prob + morph_prob + trans_prob2;
//#ifdef DEBUG
//						fprintf(stderr, "prob = %lf (max prob = %lf)\n", cur_prob, cur_max);
//#endif
//						if (cur_prob > cur_max) cur_max = cur_prob; // 최대확률 갱신
//					}
//					else // 어절 끝이 아니면
					{
						// 이전 분석 확률 + 전이 확률 + 어휘 확률
						cur_prob = table[front_tab].mresult[l].prob + trans_prob + morph_prob;

						if (cur_prob > cur_max) cur_max = cur_prob; // 최대확률 갱신
#ifdef DEBUG
						fprintf(stderr, "cur prob = %lf (max prob = %lf)\n", cur_prob, cur_max);
#endif
					}
						
					// cut-off가 가능한지 검사
					if (CutoffThreshold > 0 && cur_max - cur_prob > CutoffThreshold)
					{
#ifdef DEBUG
						fprintf(stderr, "cutoffed1\ncur_max %lf - cur_prob %lf = %lf > threshold %lf\n\n", cur_max, cur_prob, cur_max-cur_prob, CutoffThreshold);
#endif
						continue; // 여기서 중지
					}

					// 음절수에 비례하여 정한 threshold 보다 확률값이 작으면
					#ifdef SYLLABLE_ANALYSIS // 뒤에서 음절 단위 분석을 하는 경우만 검사
					if (cur_prob < AbsThreshold)
					{
#ifdef DEBUG
						fprintf(stderr, "cutoffed3\ncur_prob = %lf < AbsThreshold = %lf\n", cur_prob, AbsThreshold);
#endif
						continue;
					}
					#endif

					// 속도 향상에 상당한 기여, but 누락되는 결과들이 생길 수도 있다.
					if (cur_max - cur_prob > 15) // 10을 써보기도 했었음
					{
#ifdef DEBUG
						fprintf(stderr, "cutoffed2\n\n");
#endif
						continue;
					}

					// 저장 (결과 및 확률)
					table[total_tab].mresult[table[total_tab].count].str = strdup( result);
					table[total_tab].mresult[table[total_tab].count].prob = cur_prob;
					table[total_tab].count++;
					/**/assert( table[total_tab].count < KMATcMAX_MA_RESULT_NUM);

					table_max[total_tab] = cur_max;

#ifdef DEBUG
					if (j == NumSyll) fprintf(stderr, "완결[%d at %d]: ", table[total_tab].count-1, total_tab);
					else fprintf(stderr, "중간 : ");
							
					fprintf(stderr, "[%s\t%f]\n", result, cur_prob);
					fprintf(stderr, "\n");
#endif
				} // end of for 현재부분 (모든 품사에 대해)
			} // end of for 앞부분 (모든 분석 결과에 대해)
			
			// 정렬
			// 이것을 하게 되면 결과의 수가 줄어들고 (cutoff에 기인), 분석 시간이 약간 단축된다.
			qsort( table[total_tab].mresult, table[total_tab].count, sizeof( MAtRESULT), ma_CompareProb);
			
		} // end of for (i = j-1; i >= 1; i--)
	} // end of for (j = 2; j <= NumSyll; j++)

	// (전체 어절에 대한) 최종 분석 결과 /////////////////////////////////////////////
	cur_tab = TabPos2to1( 0, NumSyll, NumSyll);
	
	// 확률값에 따라 정렬되어 있으므로 첫번째 결과가 가장 높은 확률을 가진다고 가정
	max_prob = table_max[cur_tab];

	for (i = 0; i < table[cur_tab].count; i++)
	{
		assert( max_prob >= table[cur_tab].mresult[i].prob);
		
		if (CutoffThreshold > 0 && max_prob - table[cur_tab].mresult[i].prob > CutoffThreshold) // 임계값과의 차이 비교
		{
#ifdef DEBUG
			fprintf(stderr, "cutoffed4\n");
#endif
			break; // 더 이상 볼 것 없이 여기서 끝냄
		}

		// 결과 저장 (확률 + 분석결과)
		// 더 이상 저장할 공간이 없으면 중지
		if (Result->count >= KMATcMAX_MA_RESULT_NUM-1) break;
		
		Result->mresult[Result->count].str = strdup( table[cur_tab].mresult[i].str );
		Result->mresult[Result->count].prob = table[cur_tab].mresult[i].prob;
		Result->count++;
	}
	
	// 메모리 해제
	for (i = 0; i < NumCell; i++)
	{
		for (j = 0; j < table[i].count; j++)
		{
			if (table[i].mresult[j].str)
			{
				free( table[i].mresult[j].str);
				table[i].mresult[j].str = NULL;
			}
		}
	}
	free( table);

	return 1;
}
