/*
 * Copyright 2011-2014 Formal Methods and Tools, University of Twente
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Do not include this file directly. Instead, include sylvan.h */

#ifndef SYLVAN_SEQ_H
#define SYLVAN_SEQ_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

MDD ldd_makenode(uint32_t value, MDD ifeq, MDD ifneq);
MDD lddmc_firing_mono(MDD cmark, MDD _minus, MDD _plus);
MDD lddmc_union_mono(MDD a, MDD b);
MDD ldd_minus(MDD a, MDD b);
void convert_wholemdd_string(MDD cmark,char **result,unsigned int &length);
void convert_to_string(MDD mdd,char *chaine_mdd);

void ldd_divide(MDD mdd,const int level,MDD *mdd1,MDD *mdd2);
MDD lddmc_project_node(MDD mdd);
int get_mddnbr(MDD mdd,int level);
int isSingleMDD(MDD mdd);
MDD ldd_divide_rec(MDD a, int level);
MDD ldd_divide_internal(MDD a,int current_level,int level);

void sylvan_gc_seq();
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

