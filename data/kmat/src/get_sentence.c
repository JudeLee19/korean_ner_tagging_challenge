#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "kmat.h"
#include "kmat_def.h"

// ������ �Լ��� stdin�� �Է����� �� �� ����

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
		// ���� ���̰� �ʹ� �� ��� (������ �κ��� ����)
		if (num_word+1 >= KMATcMAX_EJ_NUM)
		{
			fprintf( stderr, "Too many words in a line! (The rest of the words will be ignored!)\n");
			return num_word;
		}

		Words[num_word++] = strdup( pch); // ���ڿ� ����

		pch = strtok( NULL, " \t\r\n");
	}
	
	return num_word;
}

///////////////////////////////////////////////////////////////////////////////
// fp�κ��� ���ڿ��� �о�鿩 word�� �����Ѵ�.
// Words[0]���� Words[���ϰ�-1]�� ä����
// ���ο��� �޸𸮸� �Ҵ��ϹǷ� �ܺο��� �ݵ�� free�ؾ� ��
// ���ٿ� �� ����
// [����] ���峻 �ܾ��� ��
int GetSentence_Row
(
	FILE	*Fp,
	char	**Words
)
{
	int num_word = 0;
	static char str[MAX_MJ];
	char *pch;

	// �� �� �б�
	while (fgets (str, MAX_MJ, Fp) != NULL) 
	{ 
		if (str[0] == '\n' || str[0] == '\r') continue; // �ƹ��͵� ������ ���� ���� �б�
		
		pch = strtok (str, " \t\r\n");
		
		while (pch != NULL) 
		{
			// ���� ���̰� �ʹ� �� ��� (������ �κ��� ����)
			if (num_word+1 >= KMATcMAX_EJ_NUM)
			{
				fprintf( stderr, "Too many words in a line! (The rest of the words will be ignored!)\n");
				return num_word;
			}

			Words[num_word++] = strdup( pch); // ���ڿ� ����

			pch = strtok(NULL, " \t\r\n");
		}
		
		if (num_word == 0) continue; // ���� ���� �õ�
		else 
		{
			return num_word;
		}
	}
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// ������ �о��
// Words[1]���� Words[num_word]�� �����
// �� �ٿ� �� �ܾ�
// �� ���� ���� ���п� ����
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
		// ���� ���̰� �ʹ� �� ��� (������ �κ��� ����)
		if (num_word+1 >= KMATcMAX_EJ_NUM)
		{
			fprintf( stderr, "Too many words in a line! (The rest of the words will be ignored!)\n");
			return num_word;
		}

		if (line[0] == '\n') // ������ ��
		{
			return num_word;
		}
		else 
		{
			line[strlen(line)-1] = 0;
			Words[num_word++] = strdup( line); // ���ڿ� ����
		}
	}
	return num_word;
}

///////////////////////////////////////////////////////////////////////////////
// return value : ������ �ܾ��
// Words[0]���� ä����
// morph_analyzed_result[1]���� ä����
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

	// ���� ����
	// �ʱ�ȭ
	Result->count = 1;
	Result->mresult[0].str = strdup( BOW_TAG_1);
	Result->mresult[0].prob = 1.0;

	while (fgets( line, MAX_MJ, Fp) != NULL) 
	{
		line_num++;

		if (line[0] == '\n') // ������ ��
		{
			if (num_word) return num_word;
		}

		if (line[0] != '\t') // ������ �����̸�
		{
			num_word++; // ���� �� ��(pre) ����
			count = 0;

			// ������, ���¼Һм����, Ȯ��
			sscanf( line, "%s%s%lf", one_word, morph_result, &prob);
			
			Words[num_word-1] = strdup( one_word);
		}
		else 
		{
			// ���¼Һм����, Ȯ��
			sscanf( line, "%s%lf", morph_result, &prob);
		}

		// Ȯ�� + �м���� ����
		Result[num_word].mresult[count].str = strdup( morph_result);
		Result[num_word].mresult[count].prob = prob;
		Result[num_word].count = ++count;
		
		
	} // end of while
	
	return num_word;
}

///////////////////////////////////////////////////////////////////////////////
// return value : ������ �ܾ��
// Words[0]���� ä����
// morph_analyzed_result[1]���� ä����
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
//	// ���� ����
//	// �ʱ�ȭ
//	Result->count = 1;
//	Result->mresult[0].str = strdup( BOW_TAG_1);
//	Result->mresult[0].prob = 1.0;
//			
//	long curfpos = ftell(Fp); // ���� ��ġ
//
//	while (fgets(line, MAX_MJ, Fp) != NULL) 
//	{
//		line_num++;
//
//		if (line[0] != '\t') // ������ �����̸�
//		{
//			if (quit) // ���� ��
//			{
//				fseek( Fp, curfpos, SEEK_SET); // ���� ��ġ �ǵ���
//				line_num--;
//				break;
//			}
//
//			num_word++;
//			count = 0;
//
//			// ������, ���¼Һм����, Ȯ��
//			sscanf( line, "%s%s%lf", one_word, morph_result, &prob);
//
//			Words[num_word-1] = strdup( one_word);
//
//			// ���� �и� ��ġ�̸�
//			if ( sbd_is_sentence_boundary( sb_fst, one_word) == 1) // ������
//			{
//				quit = 1; // ���峡 ������
//				//**/fprintf(stdout, "���峡\n");
//			}
//		}
//
//		else 
//		{
//			// ���¼Һм����, Ȯ��
//			sscanf( line, "%s%lf", morph_result, &prob);
//		}
//
//		// Ȯ�� + �м���� ����
//		Result[num_word].mresult[count].str = strdup( morph_result);
//		Result[num_word].mresult[count].prob = prob;
//		Result[num_word].count = ++count;
//			
//		curfpos = ftell( Fp);	// ���� ��ġ
//
//	} // end of while
//	
//	return num_word;
//}
