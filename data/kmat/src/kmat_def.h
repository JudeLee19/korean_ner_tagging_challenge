#ifndef __TYPEDEF_H__
#define __TYPEDEF_H__

//**/#define DEBUG // ���⿡ �����ϸ� ���� ��� �ҽ� �ڵ忡�� ����� ���� ������

#define DELIMITER '/'

// ǰ�� ���� �� ����(����ؾ� ��)
#define HMM_TAGGING

// HMM_TAGGING�� ����ϴ� ��쿡�� �ش��
// bigram, trigram �� ���� (����ؾ� ��)
//#define BIGRAM_TAGGING
#define TRIGRAM_TAGGING

// �и� ��� ���� (��� ��쿡 �ش��)
#define USING_DENOMINATOR

#define MAX_TAG_LEN 50

// ��� ����
#define BOW_TAG_2 "2|" // �������� �±� -2
#define BOW_TAG_1 "1|" // �������� �±� -1

#define BOW_SYL_2 "|2" // �������� ���� -2
#define BOW_SYL_1 "|1" // �������� ���� -1

#define EOW_TAG "$>"  // ������ �±�
#define EOW_SYL "<$"  // ������ ����

#define BOSTAG_1 ">$-1"  // ������� �±�
#define BOSTAG_2 ">$-2"

// �м� ����� ó�� �� (���� | ���¼� | ���� ���� �� | ��� ����)
#define EOJEOL 1
#define MORPHEME 2
#define SYLLABLE 4
#define NORESULT 0

#endif
