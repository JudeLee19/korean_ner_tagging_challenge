#ifndef SPLIT_WORD_H
#define SPLIT_WORD_H

extern int SplitWord
(
	const char	*Eojeol,	// [input] 어절
	int		Len,		// [input] Eojeol의 길이
	char		*SubEjs[],	// [output] 부분 어절열
	int		Ctypes[]	// [output] 문자 유형
);

#endif
