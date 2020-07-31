#include "tn5250-private.h"

/* Device control */
void scs_sic (Tn5250SCS * This);
void scs_sea (Tn5250SCS * This);
void scs_noop (Tn5250SCS * This);
void scs_rpt (Tn5250SCS * This);
void scs_sw (Tn5250SCS * This);
void scs_transparent (Tn5250SCS * This);
void scs_bel (Tn5250SCS * This);
void scs_spsu (Tn5250SCS * This);

/* Page control */
void scs_ppm (Tn5250SCS * This);
void scs_spps (Tn5250SCS * This);
void scs_shf (Tn5250SCS * This);
void scs_svf (Tn5250SCS * This);
void scs_ff (Tn5250SCS * This);
void scs_rff (Tn5250SCS * This);
void scs_sto (Tn5250SCS * This);
void scs_shm (Tn5250SCS * This);
void scs_svm (Tn5250SCS * This);
void scs_sffc (Tn5250SCS * This);

/* Font controls */
void scs_scgl (Tn5250SCS * This);
void scs_scg (Tn5250SCS * This);
void scs_sfg (Tn5250SCS * This);
void scs_scd (Tn5250SCS * This);

/* Cursor control */
void scs_pp (Tn5250SCS * This);
void scs_rdpp (Tn5250SCS * This);
void scs_ahpp (Tn5250SCS * This);
void scs_avpp (Tn5250SCS * This);
void scs_rrpp (Tn5250SCS * This);
void scs_sbs (Tn5250SCS * This);
void scs_sps (Tn5250SCS * This);
void scs_nl (Tn5250SCS * This);
void scs_irs (Tn5250SCS * This);
void scs_rnl (Tn5250SCS * This);
void scs_irt (Tn5250SCS * This);
void scs_stab (Tn5250SCS * This);
void scs_ht (Tn5250SCS * This);
void scs_it (Tn5250SCS * This);
void scs_sil (Tn5250SCS * This);
void scs_lf (Tn5250SCS * This);
void scs_cr (Tn5250SCS * This);
void scs_ssld (Tn5250SCS * This);
void scs_sld (Tn5250SCS * This);
void scs_sls (Tn5250SCS * This);

/* Generation controls */
void scs_sgea (Tn5250SCS * This);

void scs_process2b (Tn5250SCS * This);
void scs_processd2 (Tn5250SCS * This);
void scs_process03 (unsigned char nextchar, unsigned char curchar);
void scs_scs (int *cpi);
void scs_process04 (Tn5250SCS * This, unsigned char nextchar,
		    unsigned char curchar);
void scs_processd1 (Tn5250SCS * This);
void scs_process06 ();
void scs_process07 (Tn5250SCS * This);
void scs_processd103 (Tn5250SCS * This);
void scs_jtf (unsigned char curchar);
void scs_sjm (unsigned char curchar);
void scs_processd3 (Tn5250SCS * This);
void scs_setfont (Tn5250SCS * This);
void scs_main (Tn5250SCS * This);
void scs_init (Tn5250SCS * This);
void scs_default (Tn5250SCS * This);

static void scs_log(const char *msg, ...);
