#ifndef _STRVECTOR_H_
#define _STRVECTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int capacity;	// �Ҵ�� �޸� ũ��
	int count;	// ���ڿ��� ��			��) 3
	char **strs;	// ���ڿ��� ��	��) strs[0] = "ab", strs[1] = "cd", strs[2] = "ef"
} SVtSTRVCT;		// string vector

extern SVtSTRVCT *sv_New( );

extern SVtSTRVCT *sv_NewArr
(
	int NumElem
);

// ���ڿ��� �ǵڿ� �����Ѵ�.
extern void sv_Push
(
	SVtSTRVCT	*StrVct,	// [input] ���ڿ� ����
	const char	*Str
);

extern void sv_Free
(
	SVtSTRVCT	*StrVct	// [input] ���ڿ� ����
);

extern void sv_FreeArr
(
	SVtSTRVCT	*StrVctArr,	// [input]
	int		NumElem	// [input] ����� ��
);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
