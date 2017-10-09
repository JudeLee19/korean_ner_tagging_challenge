#ifndef _STRVECTOR_H_
#define _STRVECTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int capacity;	// 할당된 메모리 크기
	int count;	// 문자열의 수			예) 3
	char **strs;	// 문자열의 열	예) strs[0] = "ab", strs[1] = "cd", strs[2] = "ef"
} SVtSTRVCT;		// string vector

extern SVtSTRVCT *sv_New( );

extern SVtSTRVCT *sv_NewArr
(
	int NumElem
);

// 문자열을 맨뒤에 삽입한다.
extern void sv_Push
(
	SVtSTRVCT	*StrVct,	// [input] 문자열 벡터
	const char	*Str
);

extern void sv_Free
(
	SVtSTRVCT	*StrVct	// [input] 문자열 벡터
);

extern void sv_FreeArr
(
	SVtSTRVCT	*StrVctArr,	// [input]
	int		NumElem	// [input] 결과의 수
);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
