/* -*- C++ -*- */
#ifndef RdPMONTEUR_H
#define RdPMONTEUR_H
#include <iostream>
#include <string>
using namespace std;

class RdPMonteur {
 public:
  RdPMonteur(){};
  virtual ~RdPMonteur(){};
  virtual bool addPlace(const string &place, int marking = 0,
                        int capacity = 0) = 0;
  virtual bool addQueue(const string &place, int capacity = 0) = 0;
  virtual bool addLossQueue(const string &place, int capacity = 0) = 0;
  virtual bool addTrans(const string &transition) = 0;
  virtual bool addPre(const string &place, const string &transition,
                      int valuation = 1) = 0;
  virtual bool addPost(const string &place, const string &transition,
                       int valuation = 1) = 0;
  virtual bool addPreQueue(const string &place, const string &transition,
                           int valuation = 1) = 0;
  virtual bool addPostQueue(const string &place, const string &transition,
                            int valuation = 1) = 0;
  virtual bool addInhibitor(const string &place, const string &transition,
                            int valuation = 1) = 0;
  virtual bool addPreAuto(const string &place, const string &transition,
                          const string &valuation) = 0;
  virtual bool addPostAuto(const string &place, const string &transition,
                           const string &valuation) = 0;
  virtual bool addReset(const string &place, const string &transition) = 0;
  virtual bool create(const char *file);
};
#endif
