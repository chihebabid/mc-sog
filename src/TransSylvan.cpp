#include <cstdint>
#include <cstddef>
#include <string>
#include "SylvanWrapper.h"
#include "TransSylvan.h"

TransSylvan::TransSylvan(const MDD &_minus, const MDD &_plus):m_minus(_minus),m_plus(_plus)
{
    //ctor
}

TransSylvan::~TransSylvan()
{
    //dtor
}
void TransSylvan::setMinus(MDD _minus) {
    m_minus=_minus;
}

void TransSylvan::setPlus(MDD _plus) {
    m_plus=_plus;
}

MDD TransSylvan::getMinus() {
    return m_minus;
}

MDD TransSylvan::getPlus() {
    return m_plus;
}
