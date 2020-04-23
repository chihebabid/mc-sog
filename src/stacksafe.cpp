/*
 * Copyright 2020 chiheb <email>
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

#include "stacksafe.h"

template<typename T>
StackSafe<T>::StackSafe() {}

template<typename T>
StackSafe<T>::StackSafe ( const StackSafe& other)
{
    std::scoped_lock lock ( other.m_mutex );
    m_data=other.m_data;
}

template<typename T>
void StackSafe<T>::push ( T new_value ){
    std::scoped_lock lock ( m_mutex );
    m_data.push ( std::move ( new_value ) );
}
template<typename T>
std::shared_ptr<T> StackSafe<T>::pop()
{
    std::scoped_lock lock(m_mutex); 
    if(m_data.empty()) throw empty_stack();
    std::shared_ptr<T> const res(std::make_shared<T>(m_data.top()));
    m_data.pop();
    return res; 
}
template<typename T>
void StackSafe<T>::pop(T& value)
{
    std::scoped_lock lock(m_mutex);
    if(m_data.empty()) throw empty_stack();
    value=m_data.top();
    m_data.pop();
}
template<typename T>
bool StackSafe<T>::try_pop(T& value)
{
    std::scoped_lock lock(m_mutex);
    if(m_data.empty()) return false;
    value=m_data.top();
    m_data.pop();
    return true;
}



template class StackSafe<Pair>;
typedef pair<string *, unsigned int> MSG;
template class StackSafe<MSG>;





