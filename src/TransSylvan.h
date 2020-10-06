#ifndef TRANSSYLVAN_H
#define TRANSSYLVAN_H



class TransSylvan {
 public:
  TransSylvan(const MDD &_minus, const MDD &_plus);
  virtual ~TransSylvan();
  void setMinus(MDD _minus);
  void setPlus(MDD _plus);
  MDD getMinus();
  MDD getPlus();

 protected:
 private:
  MDD m_minus, m_plus;
};

#endif  // TRANSSYLVAN_H
