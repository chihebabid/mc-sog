/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2020  chiheb <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include "SafeDequeue.h"

template<typename T>
SafeDequeue<T>::SafeDequeue()
{

}

template<typename T>
SafeDequeue<T>::SafeDequeue ( const SafeDequeue& other )
{
    std::lock_guard<std::mutex> lk ( other.mut );
    data_queue=other.data_queue;
}

template<typename T>
SafeDequeue<T>::~SafeDequeue()
{

}

template<typename T>
void SafeDequeue<T>::push ( T new_value )
{
    std::lock_guard<std::mutex> lk ( mut );
    data_queue.push ( new_value );
    data_cond.notify_one();

}


template<typename T>
bool SafeDequeue<T>::try_pop ( T& value )
{
    std::lock_guard<std::mutex> lk ( mut );
    if ( data_queue.empty() ) {
        return false;
    }
    value=data_queue.front();
    data_queue.pop();
    return true;
}


template<typename T>
bool SafeDequeue<T>::empty() const
{
    std::lock_guard<std::mutex> lk ( mut );
    return data_queue.empty();
}

template <typename T>
T& SafeDequeue<T>::front()
{
    std::unique_lock<std::mutex> lk(mut);
    while (data_queue.empty())
    {
        data_cond.wait(lk);
    }
    return data_queue.front();
}

template <typename T>
T& SafeDequeue<T>::back()
{
    std::unique_lock<std::mutex> lk(mut);
    while (data_queue.empty())
    {
        data_cond.wait(lk);
    }
    return data_queue.back();
}

template <typename T>
int SafeDequeue<T>::size()
{
    std::unique_lock<std::mutex> lk(mut);
    int size = data_queue.size();
    lk.unlock();
    return size;
}


template class SafeDequeue<Pair>;
typedef pair<string *, unsigned int> MSG;
template class SafeDequeue<MSG>;
template class SafeDequeue<couple_th>;

template class SafeDequeue<coupleSuccessor>;
template class SafeDequeue<spot::formula>;
//typedef pair<struct myState*, int> coupleSuccessor;
template class SafeDequeue<struct myState*>;
