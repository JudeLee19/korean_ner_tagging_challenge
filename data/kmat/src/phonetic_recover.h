#ifndef _PHONETIC_RECOVER_H_
#define _PHONETIC_RECOVER_H_

#include "kmat.h"

typedef struct
{
	void	*fst;
	void	*info;
	void	*freq;
} PHONETICtRSC;

extern PHONETICtRSC* phonetic_Open
(
	char	*FST_Path,
	char	*FST_INFO_Path,
	char	*FST_FREQ_Path
);

extern void phonetic_Close
(
	PHONETICtRSC *Rsc
);

extern int phonetic_Recover
(
	PHONETICtRSC	*Rsc,
	const char		*Str,			// [input] 입력 어절
	MAtRESULTS	*RestoredStr	// [output] 음운 복원 결과
);

#endif
