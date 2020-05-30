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

#ifndef STACKSAFE_H
#define STACKSAFE_H
#include <mutex>
/**
 * @todo A thread safe stack
 */
#include "LDDState.h"
#include <exception>
#include <memory>
#include <stack>
#include <exception>
typedef pair<LDDState *, MDD> couple;
typedef pair<couple, Set> Pair;
struct empty_stack: std::exception {
    ~empty_stack() {};
    const char* what() const noexcept {return "heloo";}
    
    };
template<typename T> class StackSafe {
    private:
        std::stack<T> m_data;
        mutable std::mutex m_mutex;
    public:
        explicit StackSafe();
        ~StackSafe() {};
         StackSafe ( const StackSafe& other);
        StackSafe& operator= ( const StackSafe& ) = delete;
        void push ( T new_value );
        std::shared_ptr<T> pop();
        void pop ( T& value );
        bool try_pop ( T& value );
        bool empty() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_data.empty();
        };
    };
#endif // STACKSAFE_H
