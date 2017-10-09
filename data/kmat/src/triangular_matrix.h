#ifndef TRIANGULAR_MATRIX_H
#define TRIANGULAR_MATRIX_H

// 삼각행렬의 원소를 입력받아 일차원배열에 대응되는 위치를 리턴
// see also TabPos1to2
#define TabPos2to1(x, y, n) ((x)*(2*(n)-(x)-1)/2+(y)-1) // dglee

// 문자열의 크기를 입력받아 삼각행렬의 셀수를 리턴
#define TabNum(x) ((x)*((x)+1)/2) // added by dglee

// 삼각표 위치 구조체
typedef struct {
	int x;
	int y;
} t_TAB;

extern void setpos
(
	t_TAB *pos,
	int x,
	int y
);

extern short is_empty
(
	t_TAB pos
);

extern char *TabPos2String
(
	char *src,
	char *dst,
	t_TAB *pos
);

extern short TabPos1to2
(
	int x,
	int n,
	t_TAB *pos
);

extern int GetAllSubstring
(
	char	*SrcStr,
	int	NumChars,
	char	**AllSubstr
);

#endif
