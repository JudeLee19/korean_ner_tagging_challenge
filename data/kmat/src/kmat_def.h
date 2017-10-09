#ifndef __TYPEDEF_H__
#define __TYPEDEF_H__

//**/#define DEBUG // 여기에 선언하면 거의 모든 소스 코드에서 디버깅 모드로 동작함

#define DELIMITER '/'

// 품사 부착 모델 종류(토글해야 함)
#define HMM_TAGGING

// HMM_TAGGING을 사용하는 경우에만 해당됨
// bigram, trigram 모델 선택 (토글해야 함)
//#define BIGRAM_TAGGING
#define TRIGRAM_TAGGING

// 분모 사용 유무 (모든 경우에 해당됨)
#define USING_DENOMINATOR

#define MAX_TAG_LEN 50

// 상수 선언
#define BOW_TAG_2 "2|" // 어절시작 태그 -2
#define BOW_TAG_1 "1|" // 어절시작 태그 -1

#define BOW_SYL_2 "|2" // 어절시작 음절 -2
#define BOW_SYL_1 "|1" // 어절시작 음절 -1

#define EOW_TAG "$>"  // 어절끝 태그
#define EOW_SYL "<$"  // 어절끝 음절

#define BOSTAG_1 ">$-1"  // 문장시작 태그
#define BOSTAG_2 ">$-2"

// 분석 결과의 처리 모델 (어절 | 형태소 | 음절 단위 모델 | 결과 없음)
#define EOJEOL 1
#define MORPHEME 2
#define SYLLABLE 4
#define NORESULT 0

#endif
