#ifndef SYLVAN_SOG_H_INCLUDED
#define SYLVAN_SOG_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */




TASK_DECL_3(MDD, lddmc_firing_lace, MDD, MDD, MDD)
#define lddmc_firing_lace(cmark, minus, plus) CALL(lddmc_firing_lace, cmark, minus, plus)



#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif // SYLVAN_SOG_H_INCLUDED
