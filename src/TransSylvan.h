#ifndef TRANSSYLVAN_H
#define TRANSSYLVAN_H



class TransSylvan {
 public:
  TransSylvan(const MDD &_minus, const MDD &_plus);
  virtual ~TransSylvan();
  [[nodiscard]] MDD getMinus() const;
  [[nodiscard]] MDD getPlus() const;
 private:
    MDD m_minus, m_plus;
};

#endif  // TRANSSYLVAN_H
