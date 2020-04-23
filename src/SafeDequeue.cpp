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

#include "SafeDequeue.h"

template<typename T>
SafeDequeue<T>::SafeDequeue()
{

}

template<typename T>
SafeDequeue<T>::SafeDequeue ( const SafeDequeue& other )
{
    std::scoped_lock lk ( other.mut );
    data_queue=other.data_queue;
}

template<typename T>
SafeDequeue<T>::~SafeDequeue()
{

}

template<typename T>
void SafeDequeue<T>::push ( T new_value )
{
    std::scoped_lock lk ( mut );
    data_queue.push ( new_value );
    data_cond.notify_one();

}

/*template<typename T>
void SafeDequeue<T>::wait_and_pop ( T& value )
{
    
}*/

template<typename T>
std::shared_ptr<T> SafeDequeue<T>::wait_and_pop()
{
    std::unique_lock<std::mutex> lk ( mut );
    data_cond.wait ( lk,[this] {return !data_queue.empty();} );
    std::shared_ptr<T> res ( std::make_shared<T> ( data_queue.front() ) );
    data_queue.pop();
    return res;
}

template<typename T>
bool SafeDequeue<T>::try_pop ( T& value )
{
    std::scoped_lock lk ( mut );
    if ( data_queue.empty() ) {
        return false;
    }
    value=data_queue.front();
    data_queue.pop();
    return true;
}


template<typename T>
std::shared_ptr<T> SafeDequeue<T>::try_pop()
{
    std::scoped_lock lk ( mut );
    if ( data_queue.empty() ) {
        return std::shared_ptr<T>();
    }
    std::shared_ptr<T> res ( std::make_shared<T> ( data_queue.front() ) );
    data_queue.pop();
    return res;
}

template<typename T>
bool SafeDequeue<T>::empty() const
{
    std::scoped_lock lk ( mut );
    return data_queue.empty();
}


template class SafeDequeue<Pair>;
typedef pair<string *, unsigned int> MSG;
template class SafeDequeue<MSG>;
