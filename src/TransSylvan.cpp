#include "SylvanWrapper.h"
#include "TransSylvan.h"

TransSylvan::TransSylvan(const MDD &_minus, const MDD &_plus):m_minus(_minus),m_plus(_plus)
{
    //ctor
}

TransSylvan::~TransSylvan()=default;

MDD TransSylvan::getMinus() const {
    return m_minus;
}

MDD TransSylvan::getPlus() const {
    return m_plus;
}


